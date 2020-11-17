package main

import (
	"context"
	"errors"
	"fmt"
	"io/ioutil"
	"log"
	"time"

	pb "github.com/KitFung/tb-iot/proto"

	"gopkg.in/yaml.v2"
)

type TaskConfig struct {
	Name  string `yaml:"Name"`
	User  string `yaml:"User"`
	Cert  string `yaml:"Cert"` // The file path to the cert file
	Email string `yaml:"Email"`
	Info  struct {
		Image    string           `yaml:"Image"` // Must uploaded to the 137.189.97.26:5000 register
		Cmd      string           `yaml:"Cmd"`
		Device   map[string]int32 `yaml:"Device"` // ex: gpu:2, fpga: 2
		Sensor   []string         `yaml:"Sensor"` // Used sensor data. Just for reference right now
		NodeSpec struct {         // Affect how task allocate to node
			SensorType []string `yaml:"SensorType"` // Must in node with those sensor
			NodeName   string   `yaml:"NodeName"`   // Must in specific node
		} `yaml:"NodeSpec"`
	} `yaml:"Info"`
}

func (t *TaskConfig) Validate() error {
	if len(t.Name) == 0 {
		return errors.New("Task Name cannot be empty")
	}
	if len(t.User) == 0 {
		return errors.New("User Name cannot be empty")
	}
	if len(t.Email) == 0 || !isEmailValid(t.Email) {
		return errors.New("User Email is Invalid")
	}
	if len(t.Info.Image) == 0 {
		return errors.New("Task Image cannot be empty")
	}

	return nil
}

func NewTaskConfig(fpath string) (*TaskConfig, error) {
	dat, err := ioutil.ReadFile(fpath)
	if err != nil {
		return nil, err
	}

	t := &TaskConfig{}
	err = yaml.Unmarshal(dat, t)
	if err != nil {
		return nil, err
	}
	fmt.Println(fpath)
	fmt.Println(t)

	if err = t.Validate(); err != nil {
		return nil, err
	}

	return t, nil
}

func pb_Sensor(str string) (pb.Sensor, error) {
	switch str {
	case "CAMERA":
		return pb.Sensor_CAMERA, nil
	case "RADAR":
		return pb.Sensor_RADAR, nil
	case "LIDAR":
		return pb.Sensor_LIDAR, nil
	case "THERMAL":
		return pb.Sensor_THERMAL, nil
	}
	return 0, errors.New(fmt.Sprintf("Unknown Sensor: %s", str))
}

func NewTaskRequest(address string, taskYaml string) error {
	fmt.Println("A")

	taskConf, err := NewTaskConfig(taskYaml)
	if err != nil {
		log.Fatalf("Error while parsing the input yaml: %v", err)
	}
	fmt.Println("B")
	fmt.Println(address)
	conn, err := NewGrpcConn(address)
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()
	fmt.Println("C")

	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	fmt.Println("D")

	c := pb.NewTestBedMasterClient(conn)
	fmt.Println("E")

	Info := &pb.Task{
		Name:  &taskConf.Name,
		Image: &taskConf.Info.Image,
	}
	if len(taskConf.Info.Cmd) > 0 {
		Info.Cmd = &taskConf.Info.Cmd
	}
	if len(taskConf.Info.Device) > 0 {
		Info.Device = taskConf.Info.Device
	}
	if len(taskConf.Info.Sensor) > 0 {
		var sensor []pb.Sensor
		for _, str := range taskConf.Info.Sensor {
			t, err := pb_Sensor(str)
			if err != nil {
				log.Panic(err)
			}
			sensor = append(sensor, t)
		}
		Info.Sensor = sensor
	}
	unodespec := &taskConf.Info.NodeSpec
	if len(unodespec.SensorType) > 0 || len(unodespec.NodeName) > 0 {
		nodespec := &pb.NodeSpec{}
		if len(unodespec.SensorType) > 0 {
			var sensor []pb.Sensor
			for _, str := range taskConf.Info.Sensor {
				t, err := pb_Sensor(str)
				if err != nil {
					log.Panic(err)
				}
				sensor = append(sensor, t)
			}
			nodespec.SensorType = sensor
		}
		if len(unodespec.NodeName) > 0 {
			nodespec.NodeName = &unodespec.NodeName
		}
	}

	req := &pb.NewTaskRequest{
		TaskName:  &taskConf.Name,
		UserName:  &taskConf.User,
		UserEmail: &taskConf.Email,
		TaskInfo:  Info,
	}

	fmt.Println("Submit new task")
	resp, err := c.NewTask(ctx, req)

	fmt.Printf("%v\n", resp)
	return err
}
