package main

import (
	"context"
	"os"
	"time"

	"github.com/coreos/etcd/clientv3"
	pluginapi "k8s.io/kubelet/pkg/apis/deviceplugin/v1beta1"
)

type Sensor struct {
	pluginapi.Device
	resourceName string
	fullName     string
}

type ResourceManager interface {
	Sensors() []*Sensor
	CheckHealth(stop <-chan interface{}, sensors []*Sensor, unhealthy chan<- *Sensor)
}
type SensorManager struct {
	etcdClient *clientv3.Client
}

func NewSensorManager() *SensorManager {
	cli, _ := clientv3.New(clientv3.Config{
		Endpoints:   []string{os.Getenv("NODE_IP") + ":2379"},
		DialTimeout: 5 * time.Second,
	})
	return &SensorManager{
		etcdClient: cli,
	}
}

func buildSensor(resourceName string, fullName string) *Sensor {
	sensor := &Sensor{
		resourceName: resourceName,
		fullName:     fullName,
	}
	sensor.ID = fullName
	sensor.Health = pluginapi.Healthy
	return sensor
}

// Later setup etcd in fog, and let it register to fog
func (m *SensorManager) CheckHealth(stop <-chan interface{}, sensors []*Sensor, unhealthy chan<- *Sensor) {
	for {
		select {
		case <-stop:
			return
		default:
		}
		for _, s := range sensors {
			ctx, cancel := context.WithTimeout(context.Background(), 1*time.Second)
			_, err := m.etcdClient.Get(ctx, s.fullName)
			cancel()
			if err != nil {
				unhealthy <- s
			}
		}
	}
}
