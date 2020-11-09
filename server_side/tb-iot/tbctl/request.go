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
	taskName  string
	userName  string
	userCert  string // The file path to the cert file
	userEmail string
	taskInfo  struct {
		image    string // Must uploaded to the 137.189.97.26:5000 register
		cmd      string
		device   map[string]int32 // ex: gpu:2, fpga: 2
		sensor   []string         // Used sensor data. Just for reference right now
		nodeSpec struct {         // Affect how task allocate to node
			sensorType []string // Must in node with those sensor
			nodeName   string   // Must in specific node
		}
	}
}

type TaskCreationRequest struct {
}

func (t *TaskConfig) Validate() error {
	if len(t.taskName) == 0 {
		return errors.New("Task Name cannot be empty")
	}
	if len(t.userName) == 0 {
		return errors.New("User Name cannot be empty")
	}
	if len(t.userEmail) == 0 || !isEmailValid(t.userEmail) {
		return errors.New("User Email is Invalid")
	}
	if len(t.taskInfo.image) == 0 {
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
	err = yaml.Unmarshal(dat, &t)
	if err != nil {
		return nil, err
	}
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
	taskConf, err := NewTaskConfig(taskYaml)
	if err != nil {
		log.Fatalf("Error while parsing the input yaml: %v", err)
	}

	conn, err := NewGrpcConn(address)
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()

	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()

	c := pb.NewTestBedMasterClient(conn)

	taskInfo := &pb.Task{
		Image: &taskConf.taskInfo.image,
	}
	if len(taskConf.taskInfo.cmd) > 0 {
		taskInfo.Cmd = &taskConf.taskInfo.cmd
	}
	if len(taskConf.taskInfo.device) > 0 {
		taskInfo.Device = taskConf.taskInfo.device
	}
	if len(taskConf.taskInfo.sensor) > 0 {
		var sensor []pb.Sensor
		for _, str := range taskConf.taskInfo.sensor {
			t, err := pb_Sensor(str)
			if err != nil {
				log.Panic(err)
			}
			sensor = append(sensor, t)
		}
		taskInfo.Sensor = sensor
	}
	uNodeSpec := &taskConf.taskInfo.nodeSpec
	if len(uNodeSpec.sensorType) > 0 || len(uNodeSpec.nodeName) > 0 {
		nodeSpec := &pb.NodeSpec{}
		if len(uNodeSpec.sensorType) > 0 {
			var sensor []pb.Sensor
			for _, str := range taskConf.taskInfo.sensor {
				t, err := pb_Sensor(str)
				if err != nil {
					log.Panic(err)
				}
				sensor = append(sensor, t)
			}
			nodeSpec.SensorType = sensor
		}
		if len(uNodeSpec.nodeName) > 0 {
			nodeSpec.NodeName = &uNodeSpec.nodeName
		}
	}

	req := &pb.NewTaskRequest{
		TaskName:  &taskConf.taskName,
		UserName:  &taskConf.userName,
		UserEmail: &taskConf.userEmail,
		TaskInfo:  taskInfo,
	}
	resp, err := c.NewTask(ctx, req)

	print("%v\n", resp)
	return err
}
