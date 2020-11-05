package main

import (
	"context"
	"fmt"
	"net"
	"os"
	"path"
	"reflect"
	"time"

	"github.com/coreos/etcd/clientv3"
	log "github.com/sirupsen/logrus"
	"google.golang.org/grpc"
	pluginapi "k8s.io/kubelet/pkg/apis/deviceplugin/v1beta1"
)

const (
	resourceNamePrefix = "cuhk.com/sensor"
	serverSockPath     = pluginapi.DevicePluginPath
)

type SensorPlugin struct {
	SensorManager
	resource string
	socket   string

	plguinServer  *grpc.Server
	centralServer *grpc.Server
	cachedSensors map[string]*Sensor
	update        chan map[string]*Sensor
	stop          chan interface{}
}

type SensorPlugins struct {
	etcdClient *clientv3.Client
	plugins    map[string]*SensorPlugin
	update     chan map[string]map[string]*Sensor
	fullNames  chan []string
}

func NewSensorPlugins() (*SensorPlugins, []string) {
	cli, _ := clientv3.New(clientv3.Config{
		Endpoints:   []string{os.Getenv("NODE_IP") + ":2379"},
		DialTimeout: 5 * time.Second,
	})
	update := make(chan map[string]map[string]*Sensor)
	p, fns := GetPlugins(cli)
	plugins := &SensorPlugins{
		etcdClient: cli,
		plugins:    p,
		update:     update,
		fullNames:  make(chan []string),
	}

	go func(p *SensorPlugins) {
		for {
			var fullNames []string
			gsensors := make(map[string]map[string]*Sensor)
			sensors, err := GetSensors(p.etcdClient)
			if err != nil {
				log.Errorf("Error to get sensors: %v", err)
				break
			}
			for _, s := range sensors {
				fullName := s.fullName
				resource := s.resource
				fullNames = append(fullNames, fullName)
				if gsensors[resource] == nil {
					gsensors[resource] = make(map[string]*Sensor)
				}
				gsensors[resource][fullName] = s
			}
			p.update <- gsensors
			p.fullNames <- fullNames
			time.Sleep(5 * time.Second)
		}
		close(p.update)
		close(p.fullNames)
	}(plugins)
	return plugins, fns
}

func (m *SensorPlugins) CheckUpdate(sensors map[string]map[string]*Sensor) {
	added := make(map[string]map[string]*Sensor)
	updated := make(map[string]map[string]*Sensor)
	removed := make(map[string]map[string]*Sensor)

	for resource, plugin := range m.plugins {
		if nsensors, ok := sensors[resource]; ok {
			if !reflect.DeepEqual(plugin.cachedSensors, nsensors) {
				updated[resource] = nsensors
			}
			delete(sensors, resource)
		} else {
			removed[resource] = plugin.cachedSensors
		}
	}
	for resource, sensors := range sensors {
		added[resource] = sensors
	}

	for resource, sensors := range added {
		plugin := NewSensorPlugin(
			*NewSensorManager(),
			resource,
			serverSockPath+"cuhk-"+resource+".sock",
			sensors)

		err := plugin.Start()
		if err != nil {
			log.Panic(err)
		}
		m.plugins[resource] = plugin
		m.plugins[resource].cachedSensors = sensors
		m.plugins[resource].update <- sensors
	}

	for resource, sensors := range removed {
		log.Printf("Remove sensor: %v", sensors)
		m.plugins[resource].Stop()
		delete(m.plugins, resource)
	}

	for resource, sensors := range updated {
		m.plugins[resource].cachedSensors = sensors
		m.plugins[resource].update <- sensors
	}
}

func (m *SensorPlugins) Stop() {
	for _, p := range m.plugins {
		p.Stop()
	}
}

func (m *SensorPlugins) Start() error {
	log.Printf("Starting %d plugin", len(m.plugins))
	for _, p := range m.plugins {
		if err := p.Start(); err != nil {
			return err
		}
	}
	return nil
}

func GetPlugins(cli *clientv3.Client) (map[string]*SensorPlugin, []string) {
	var plugins = make(map[string]*SensorPlugin)
	var fullNames []string
	sensors, err := GetSensors(cli)
	if err != nil {
		log.Fatal(err)
	}

	for _, sensor := range sensors {
		resource := sensor.resource
		fullName := sensor.fullName
		fullNames = append(fullNames, fullName)
		if val, ok := plugins[resource]; ok {
			val.cachedSensors[fullName] = sensor
		} else {
			plugins[resource] = NewSensorPlugin(
				*NewSensorManager(),
				resource,
				serverSockPath+"cuhk-"+resource+".sock",
				map[string]*Sensor{fullName: sensor})
		}
	}
	return plugins, fullNames
}

func NewSensorPlugin(sensorsManager SensorManager, resource string, socket string, cachedSensors map[string]*Sensor) *SensorPlugin {
	return &SensorPlugin{
		SensorManager: sensorsManager,
		resource:      resource,
		socket:        socket,
		cachedSensors: cachedSensors,
	}
}

func (m *SensorPlugin) initialize() {
	// m.cachedSensors = m.Devices()
	m.plguinServer = grpc.NewServer([]grpc.ServerOption{}...)
	m.centralServer = grpc.NewServer([]grpc.ServerOption{}...)
	m.update = make(chan map[string]*Sensor)
	m.stop = make(chan interface{})
}

func (m *SensorPlugin) cleanup() {
	close(m.stop)
	m.cachedSensors = nil
	m.plguinServer = nil
	m.centralServer = nil
	m.update = nil
	m.stop = nil
}

func (m *SensorPlugin) Start() error {
	log.Println("Starting plugin")

	m.initialize()
	log.Println("Initialize the plugin")

	err := m.Serve()
	if err != nil {
		log.Printf("Could not start device plugin for '%s': %s", m.resource, err)
		m.cleanup()
		return err
	}

	log.Printf("Starting to serve '%s' on %s", m.resource, m.socket)
	err = m.Register()
	if err != nil {
		log.Printf("Could not register device plugin: %s", err)
		m.Stop()
		return err
	}
	log.Printf("Registered device plugin for '%s' with Kubelet", m.resource)

	return nil
}

func (m *SensorPlugin) Stop() error {
	if m == nil || m.plguinServer == nil {
		return nil
	}
	log.Printf("Stopping to serve '%s' on %s", m.resource, m.socket)
	m.plguinServer.Stop()
	if m.centralServer != nil {
		m.centralServer.Stop()
	}

	if err := os.Remove(m.socket); err != nil && !os.IsNotExist(err) {
		return err
	}
	m.cleanup()
	return nil
}

func (m *SensorPlugin) ServeCentralServer() error {
	return nil
}

func (m *SensorPlugin) Serve() error {
	m.ServeCentralServer()

	os.Remove(m.socket)
	sock, err := net.Listen("unix", m.socket)
	if err != nil {
		return err
	}
	pluginapi.RegisterDevicePluginServer(m.plguinServer, m)

	go func() {
		lastCrashTime := time.Now()
		restartCount := 0
		for {
			log.Printf("Starting GRPC server for '%s'", m.resource)
			err := m.plguinServer.Serve(sock)
			if err == nil {
				break
			}

			log.Printf("GRPC server for '%s' crashed with error: %v", m.resource, err)

			if restartCount > 5 {
				log.Fatalf("GRPC server for '%s' has repeatedly crashed recently. Quitting", m.resource)
			}

			timeSinceLastCrash := time.Since(lastCrashTime).Seconds()
			lastCrashTime = time.Now()
			if timeSinceLastCrash > 3600 {
				restartCount = 1
			} else {
				restartCount++
			}
		}
	}()

	// Wait for server to start by launching a blocking connexion
	conn, err := m.dial(m.socket, 5*time.Second)
	if err != nil {
		return err
	}
	conn.Close()

	return nil
}

func (m *SensorPlugin) Register() error {
	conn, err := m.dial(pluginapi.KubeletSocket, 5*time.Second)
	if err != nil {
		return err
	}
	defer conn.Close()

	cli := pluginapi.NewRegistrationClient(conn)
	reqt := &pluginapi.RegisterRequest{
		Version:      pluginapi.Version,
		Endpoint:     path.Base(m.socket),
		ResourceName: ResourceName(m.resource),
		Options:      &pluginapi.DevicePluginOptions{},
	}

	_, err = cli.Register(context.Background(), reqt)
	if err != nil {
		return err
	}
	return nil
}

func (m *SensorPlugin) GetDevicePluginOptions(context.Context, *pluginapi.Empty) (*pluginapi.DevicePluginOptions, error) {
	return &pluginapi.DevicePluginOptions{}, nil
}

func (m *SensorPlugin) ListAndWatch(e *pluginapi.Empty, s pluginapi.DevicePlugin_ListAndWatchServer) error {
	for {
		select {
		case <-m.stop:
			return nil
		case d := <-m.update:
			// FIXME: there is no way to recover from the Unhealthy state.
			// d.Health = pluginapi.Unhealthy
			devices := m.apiDevices(d)
			log.Printf("ListAndWatch with %d device\n", len(devices))
			s.Send(&pluginapi.ListAndWatchResponse{Devices: devices})
		}
	}
}

func (m *SensorPlugin) Allocate(ctx context.Context, reqs *pluginapi.AllocateRequest) (*pluginapi.AllocateResponse, error) {
	responses := pluginapi.AllocateResponse{}
	for _, req := range reqs.ContainerRequests {
		for _, id := range req.DevicesIDs {
			if !m.deviceExists(id) {
				return nil, fmt.Errorf("invalid allocation request for '%s': unknown device: %s", m.resource, id)
			}
		}

		response := pluginapi.ContainerAllocateResponse{}
		responses.ContainerResponses = append(responses.ContainerResponses, &response)
	}

	return &responses, nil
}

func (m *SensorPlugin) PreStartContainer(context.Context, *pluginapi.PreStartContainerRequest) (*pluginapi.PreStartContainerResponse, error) {
	return &pluginapi.PreStartContainerResponse{}, nil
}

func (m *SensorPlugin) dial(unixSocketPath string, timeout time.Duration) (*grpc.ClientConn, error) {
	c, err := grpc.Dial(unixSocketPath, grpc.WithInsecure(), grpc.WithBlock(),
		grpc.WithTimeout(timeout),
		grpc.WithDialer(func(addr string, timeout time.Duration) (net.Conn, error) {
			return net.DialTimeout("unix", addr, timeout)
		}),
	)

	if err != nil {
		return nil, err
	}

	return c, nil
}

func (m *SensorPlugin) deviceExists(id string) bool {
	for _, d := range m.cachedSensors {
		if d.ID == id {
			return true
		}
	}
	return false
}

func (m *SensorPlugin) apiDevices(sensors map[string]*Sensor) []*pluginapi.Device {
	var pdevs []*pluginapi.Device
	for _, d := range sensors {
		pdevs = append(pdevs, &d.Device)
	}
	return pdevs
}
