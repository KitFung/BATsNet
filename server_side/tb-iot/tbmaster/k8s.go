package main

import (
	"context"
	"log"
	"strings"
	"sync"

	pb "github.com/KitFung/tb-iot/proto"
	apiv1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/rest"
)

const K8SMasterLabel = "node-role.kubernetes.io/master"

// Using k8s status as db to search status
type K8SConn struct {
	clientSet *kubernetes.Clientset
}

var k8s *K8SConn
var once sync.Once

func GetK8SConn() *K8SConn {
	once.Do(func() {
		config, err := rest.InClusterConfig()
		if err != nil {
			log.Panic(err)
		}
		clientSet, err := kubernetes.NewForConfig(config)
		if err != nil {
			log.Panic(err)
		}
		k8s = &K8SConn{
			clientSet: clientSet,
		}
	})
	return k8s
}

func GetSensors(labels []string) ([]string, []pb.Sensor) {
	var sensors []string
	var sensorsType []pb.Sensor
	sensorsTypeMap := make(map[string]bool)
	sensorNameMap := map[string]pb.Sensor{
		"camera":  pb.Sensor_CAMERA,
		"thermal": pb.Sensor_THERMAL,
		"lidar":   pb.Sensor_LIDAR,
		"radar":   pb.Sensor_RADAR,
	}
	for _, label := range labels {
		if strings.HasPrefix(label, "sensor-") {
			eles := strings.Split(label, "/")
			sensors = append(sensors, eles[1])
			typeStr := strings.Split(eles[0], "-")[1]
			if s, ok := sensorNameMap[typeStr]; ok {
				if _, ok := sensorsTypeMap[typeStr]; !ok {
					sensorsType = append(sensorsType, s)
					sensorsTypeMap[typeStr] = true
				}
			} else {
				log.Panicf("Unknown Sensor Type: %s\n", typeStr)
			}
		}
	}

	return sensors, sensorsType
}

func (m *K8SConn) DiscoveryNetwork(network *TBNetwork) {
	// network.tasks = make(map[string]*pb.Task)
	network.nodes = make(map[string]*pb.Node)

	nodes, err := m.clientSet.CoreV1().Nodes().List(context.TODO(), metav1.ListOptions{})
	if err != nil {
		log.Panic(err)
	}
	pods, err := m.clientSet.CoreV1().Pods("task").List(context.TODO(), metav1.ListOptions{})
	if err != nil {
		log.Panic(err)
	}

	tasks := network.tasks
	unknownStatus := string(apiv1.PodUnknown)
	for _, t := range tasks {
		t.Status = &unknownStatus
	}
	for _, pod := range pods.Items {
		pharse := string(pod.Status.Phase)

		task := tasks[pod.Name]
		task.Status = &pharse
		// cmd := strings.Join(pod.Spec.Containers[0].Command, " ")
		// spec := pod.Spec.Volumes[0]
		// task := &pb.Task{
		// 	Image:  &spec.Name,
		// 	Cmd:    &cmd,
		// 	Status: &pharse,
		// }
		// pod.Status.HostIP
		network.tasks[pod.Name] = task
	}

	for _, node := range nodes.Items {
		labels := node.Labels
		master := false
		for _, label := range labels {
			if label == K8SMasterLabel {
				master = true
				break
			}
		}
		if !master {
			status := &node.Status
			isReady := false

			for _, cond := range status.Conditions {
				if cond.Type == apiv1.NodeReady {
					isReady = cond.Status == apiv1.ConditionTrue
				}
			}
			nodeStatus := pb.Node_OFFLINE
			if isReady {
				nodeStatus = pb.Node_ONLINE
			}
			pbnode := &pb.Node{
				Name:   &node.Name,
				Status: &nodeStatus,
			}
			var labels []string
			for k, _ := range node.Labels {
				labels = append(labels, k)
			}
			pbnode.OnlineSensor, pbnode.OnlineSensorType = GetSensors(labels)

			network.nodes[node.Name] = pbnode
		}
	}
}
