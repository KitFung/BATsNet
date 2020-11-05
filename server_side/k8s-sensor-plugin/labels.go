package main

import (
	"context"
	"fmt"
	"log"
	"os"
	"strings"

	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/types"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/rest"
)

type LabelsUpdater struct {
	clientSet *kubernetes.Clientset
	nodeName  string
}

func NewLabelUpdater() (*LabelsUpdater, error) {
	config, err := rest.InClusterConfig()
	if err != nil {
		return nil, err
	}
	clientSet, err := kubernetes.NewForConfig(config)
	if err != nil {
		return nil, err
	}
	return &LabelsUpdater{
		clientSet: clientSet,
		nodeName:  os.Getenv("NODE_NAME"),
	}, nil
}

func FullNameToLabel(fullName string) string {
	fullName = strings.ReplaceAll(fullName, "/", ".")
	fullName = strings.TrimLeft(fullName, ".")
	return "sensor~1" + fullName
}

func (m *LabelsUpdater) ConstructPatchJson(addLabels []string, delLabels []string) string {
	patchs := make([]string, len(addLabels)+len(delLabels))
	i := 0
	for _, label := range addLabels {
		patchs[i] = fmt.Sprintf(`{"op":"add","path":"/metadata/labels/%s","value":"on" }`, label)
		i += 1
	}
	for _, label := range delLabels {
		patchs[i] = fmt.Sprintf(`{"op":"remove","path":"/metadata/labels/%s" }`, label)
		i += 1
	}

	return "[" + strings.Join(patchs, ",") + "]"
}

func (m *LabelsUpdater) UpdateLabels(fullNames []string) error {
	node, err := m.clientSet.CoreV1().Nodes().Get(context.TODO(), m.nodeName, metav1.GetOptions{})
	if err != nil {
		return err
	}
	var add []string
	var del []string

	oldSensorLabels := make(map[string]bool)
	for k := range node.Labels {
		if strings.HasPrefix(k, "sensor/") {
			oldSensorLabels[strings.ReplaceAll(k, "/", "~1")] = true
		}
	}

	for _, name := range fullNames {
		label := FullNameToLabel(name)
		if _, ok := oldSensorLabels[label]; !ok {
			add = append(add, label)
		} else {
			delete(oldSensorLabels, label)
		}
	}

	for k := range oldSensorLabels {
		del = append(del, k)
	}

	labelPatch := m.ConstructPatchJson(add, del)
	log.Println(labelPatch)
	_, err = m.clientSet.CoreV1().Nodes().Patch(
		context.TODO(), m.nodeName, types.JSONPatchType,
		[]byte(labelPatch), metav1.PatchOptions{})
	if err != nil {
		return err
	}

	return nil
}
