package main

import (
	"context"
	"time"

	"google.golang.org/grpc"
	pluginapi "k8s.io/kubelet/pkg/apis/deviceplugin/v1beta1"
)

type SensorPlugin struct {
	SensorsManager
	resourceName string
	socket       string

	server        *grpc.Server
	cachedSensors []*Sensor
	updateChan    chan *Sensor
	stop          chan interface{}
}

func GetPlugins() []*SensorPlugin {
	var plugins []*SensorPlugin
	return plugins
}

func NewSensorPlugin() *SensorPlugin {
	return &SensorPlugin{}
}

func (m *SensorPlugin) initialize() {
}

func (m *SensorPlugin) cleanup() {
}

func (m *SensorPlugin) Start() error {
	return nil
}

func (m *SensorPlugin) Stop() error {
	return nil
}

func (m *SensorPlugin) Serve() error {
	return nil
}

func (m *SensorPlugin) Register() error {
	return nil
}

func (m *SensorPlugin) GetDevicePluginOptions(context.Context, *pluginapi.Empty) (*pluginapi.DevicePluginOptions, error) {
	return nil, nil
}

func (m *SensorPlugin) ListAndWatch(e *pluginapi.Empty, s pluginapi.DevicePlugin_ListAndWatchServer) error {
	return nil
}

func (m *SensorPlugin) GetPreferredAllocation(ctx context.Context, r *pluginapi.PreferredAllocationRequest) (*pluginapi.PreferredAllocationResponse, error) {
	return nil, nil
}

func (m *SensorPlugin) Allocate(ctx context.Context, reqs *pluginapi.AllocateRequest) (*pluginapi.AllocateResponse, error) {
	return nil, nil
}

func (m *SensorPlugin) PreStartContainer(context.Context, *pluginapi.PreStartContainerRequest) (*pluginapi.PreStartContainerResponse, error) {
	return nil, nil
}

func (m *SensorPlugin) dial(unixSocketPath string, timeout time.Duration) (*grpc.ClientConn, error) {
	return nil, nil
}

func (m *SensorPlugin) deviceExists(id string) bool {
	return true
}
