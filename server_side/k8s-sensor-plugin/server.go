package main

import (
	"context"
	"fmt"
	"log"
	"net"
	"os"
	"path"
	"strings"
	"time"

	"github.com/coreos/etcd/clientv3"
	"google.golang.org/grpc"
	pluginapi "k8s.io/kubelet/pkg/apis/deviceplugin/v1beta1"
)

const (
	resourceNamePrefix = "cuhk.com/sensor"
	serverSockPath     = pluginapi.DevicePluginPath
)

type SensorPlugin struct {
	SensorManager
	resourceName string
	fullName     string
	socket       string

	plguinServer  *grpc.Server
	centralServer *grpc.Server
	cachedSensors []*Sensor
	health        chan *Sensor
	stop          chan interface{}
}

type SensorPlugins struct {
	etcdClient *clientv3.Client
	plugins    []*SensorPlugin
}

func NewSensorPlugins() *SensorPlugins {
	cli, _ := clientv3.New(clientv3.Config{
		Endpoints:   []string{"localhost:2379"},
		DialTimeout: 5 * time.Second,
	})
	return &SensorPlugins{
		etcdClient: cli,
		plugins:    GetPlugins(cli),
	}
}

func (m *SensorPlugins) Stop() {
	for _, p := range m.plugins {
		p.Stop()
	}
}

func (m *SensorPlugins) Start() error {
	for _, p := range m.plugins {
		if err := p.Start(); err != nil {
			return err
		}
	}
	return nil
}

func GetPlugins(cli *clientv3.Client) []*SensorPlugin {
	var plugins map[string]*SensorPlugin

	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	resp, err := cli.Get(ctx, "/_control",
		clientv3.WithRange(clientv3.GetPrefixRangeEnd("/_control")),
		clientv3.WithSort(clientv3.SortByKey, clientv3.SortAscend))
	cancel()

	if err != nil {
		log.Fatal(err)
	}

	for _, ev := range resp.Kvs {
		key := string(ev.Key)
		eles := strings.Split(key, "/")
		if len(eles) >= 2 && len(eles[1]) > 0 {
			resource := eles[1]
			resourceName := resourceNamePrefix + resource
			fullName := "/" + strings.Join(eles[1:], "/")
			if val, ok := plugins[resourceName]; ok {
				val.cachedSensors = append(
					val.cachedSensors, buildSensor(resourceName, fullName))
			} else {
				plugins[resourceName] = NewSensorPlugin(
					*NewSensorManager(),
					resourceName,
					fullName,
					serverSockPath+"cuhk-"+resource+".sock",
					[]*Sensor{buildSensor(resourceName, fullName)})
			}
		}
		// fmt.Printf("%s : %s\n", ev.Key, ev.Value)
	}

	pluginsList := make([]*SensorPlugin, 0, len(plugins))
	for _, plugin := range plugins {
		pluginsList = append(pluginsList, plugin)
	}
	return pluginsList
}

func NewSensorPlugin(sensorsManager SensorManager, resourceName string, fullName string, socket string, cachedSensors []*Sensor) *SensorPlugin {
	return &SensorPlugin{
		SensorManager: sensorsManager,
		resourceName:  resourceName,
		fullName:      fullName,
		socket:        socket,
		cachedSensors: cachedSensors,
	}
}

func (m *SensorPlugin) initialize() {
	// m.cachedSensors = m.Devices()
	m.plguinServer = grpc.NewServer([]grpc.ServerOption{}...)
	m.centralServer = grpc.NewServer([]grpc.ServerOption{}...)
	m.health = make(chan *Sensor)
	m.stop = make(chan interface{})
}

func (m *SensorPlugin) cleanup() {
	close(m.stop)
	m.cachedSensors = nil
	m.plguinServer = nil
	m.centralServer = nil
	m.health = nil
	m.stop = nil
}

func (m *SensorPlugin) Start() error {
	m.initialize()

	err := m.Serve()
	if err != nil {
		log.Printf("Could not start device plugin for '%s': %s", m.resourceName, err)
		m.cleanup()
		return err
	}

	log.Printf("Starting to serve '%s' on %s", m.resourceName, m.socket)
	err = m.Register()
	if err != nil {
		log.Printf("Could not register device plugin: %s", err)
		m.Stop()
		return err
	}
	log.Printf("Registered device plugin for '%s' with Kubelet", m.resourceName)

	go m.CheckHealth(m.stop, m.cachedSensors, m.health)
	return nil
}

func (m *SensorPlugin) Stop() error {
	if m == nil || m.plguinServer == nil {
		return nil
	}
	log.Printf("Stopping to serve '%s' on %s", m.resourceName, m.socket)
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
			log.Printf("Starting GRPC server for '%s'", m.resourceName)
			err := m.plguinServer.Serve(sock)
			if err == nil {
				break
			}

			log.Printf("GRPC server for '%s' crashed with error: %v", m.resourceName, err)

			if restartCount > 5 {
				log.Fatalf("GRPC server for '%s' has repeatedly crashed recently. Quitting", m.resourceName)
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
		ResourceName: m.resourceName,
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
	s.Send(&pluginapi.ListAndWatchResponse{Devices: m.apiDevices()})

	for {
		select {
		case <-m.stop:
			return nil
		case d := <-m.health:
			// FIXME: there is no way to recover from the Unhealthy state.
			d.Health = pluginapi.Unhealthy
			log.Printf("'%s' device marked unhealthy: %s", m.resourceName, d.ID)
			s.Send(&pluginapi.ListAndWatchResponse{Devices: m.apiDevices()})
		}
	}
}

func (m *SensorPlugin) Allocate(ctx context.Context, reqs *pluginapi.AllocateRequest) (*pluginapi.AllocateResponse, error) {
	responses := pluginapi.AllocateResponse{}
	for _, req := range reqs.ContainerRequests {
		for _, id := range req.DevicesIDs {
			if !m.deviceExists(id) {
				return nil, fmt.Errorf("invalid allocation request for '%s': unknown device: %s", m.resourceName, id)
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

func (m *SensorPlugin) apiDevices() []*pluginapi.Device {
	var pdevs []*pluginapi.Device
	for _, d := range m.cachedSensors {
		pdevs = append(pdevs, &d.Device)
	}
	return pdevs
}
