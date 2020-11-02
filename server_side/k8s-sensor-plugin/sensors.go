package main

type Sensor struct {
	typeId string
}

type ResourceManager interface {
	Sensors() []*Sensor
	CheckHealth(stop <-chan interface{}, sensors []*Sensor, unhealthy chan<- *Sensor)
}
type SensorsManager struct{}

// Not getting the detail sensor id, just get it type
// Example: /camera /lidar
func (m *SensorsManager) Sensors() []*Sensor {
	var sensors []*Sensor
	return sensors
}

// Later setup etcd in fog, and let it register to fog
func (m *SensorsManager) CheckHealth(stop <-chan interface{}, sensors []*Sensor, unhealthy chan<- *Sensor) {

}
