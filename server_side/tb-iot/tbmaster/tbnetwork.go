package main

import (
	pb "github.com/KitFung/tb-iot/proto"
	apiv1 "k8s.io/api/core/v1"
)

const statusLog = "/opt/tbmaster/binlog"

// Maintain the TB status
// - nodes
// - connection between nodes
// - task on nodes
// - notify the task

type LamppostNetworkTraffic struct {
}

type LamppostNetworkManager struct {
	assocNodes  map[string][]string
	regServices map[string]string
	traffic     *LamppostNetworkTraffic
}

type TBNetwork struct {
	nodes      map[string]*pb.Node
	tasks      map[string]*pb.Task
	taskAssign map[string]string

	regImages []string
	lpnm      *LamppostNetworkManager
}

type TasksGroup struct {
	pending []*pb.Task
	running []*pb.Task
	success []*pb.Task
	fail    []*pb.Task
}

func NewTBNetwork() *TBNetwork {
	return &TBNetwork{
		lpnm: &LamppostNetworkManager{},
	}
}

func (m *TBNetwork) Update() {
	GetK8SConn().DiscoveryNetwork(m)
	m.lpnm.Update()
}

func (m *TBNetwork) GetGroupedTasks() *TasksGroup {
	g := &TasksGroup{}
	for _, t := range m.tasks {
		switch apiv1.PodPhase(*t.Status) {
		case apiv1.PodPending:
			g.pending = append(g.pending, t)
			break
		case apiv1.PodRunning:
			g.running = append(g.running, t)
			break
		case apiv1.PodSucceeded:
			g.success = append(g.success, t)
			break
		case apiv1.PodFailed:
		case apiv1.PodUnknown:
		default:
			g.fail = append(g.fail, t)
			break
		}
	}
	return g
}

func (m *LamppostNetworkManager) Update() {

}
