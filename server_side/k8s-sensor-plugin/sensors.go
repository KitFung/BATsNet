package main

import (
	"context"
	"os"
	"strings"
	"time"

	"github.com/coreos/etcd/clientv3"
	pluginapi "k8s.io/kubelet/pkg/apis/deviceplugin/v1beta1"
)

type Sensor struct {
	pluginapi.Device
	resource string
	fullName string
}

type ResourceManager interface {
	Sensors() []*Sensor
	CheckHealth(stop <-chan interface{}, sensors []*Sensor, unhealthy chan<- *Sensor)
}
type SensorManager struct {
	etcdClient *clientv3.Client
}

func ResourceName(resource string) string {
	return resourceNamePrefix + "-" + resource
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

func GetSensors(cli *clientv3.Client) (map[string]*Sensor, error) {
	sensors := make(map[string]*Sensor)
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	resp, err := cli.Get(ctx, "/_control",
		clientv3.WithRange(clientv3.GetPrefixRangeEnd("/_control")),
		clientv3.WithSort(clientv3.SortByKey, clientv3.SortAscend))
	cancel()
	if err != nil {
		return nil, err
	}

	for _, ev := range resp.Kvs {
		key := string(ev.Key)
		eles := strings.Split(key, "/")
		if len(eles) > 2 && len(eles[2]) > 0 {
			resource := eles[2]
			fullName := "/" + strings.Join(eles[2:], "/")
			sensors[fullName] = buildSensor(resource, fullName)
		}
	}
	return sensors, nil
}

func buildSensor(resource string, fullName string) *Sensor {
	sensor := &Sensor{
		resource: resource,
		fullName: fullName,
	}
	sensor.ID = fullName
	sensor.Health = pluginapi.Healthy
	return sensor
}
