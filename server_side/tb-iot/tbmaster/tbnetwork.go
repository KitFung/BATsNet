package main

import (
	"bufio"
	"encoding/binary"
	"log"
	"os"
	"sync"

	pb "github.com/KitFung/tb-iot/proto"
	"google.golang.org/protobuf/proto"
	apiv1 "k8s.io/api/core/v1"
)

const statusLog = "/opt/tbmaster/binlog"
const taskFolder = "/opt/tbmaster/task"

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

	taskF *os.File
	mu    sync.Mutex
}

type TasksGroup struct {
	pending []*pb.Task
	running []*pb.Task
	success []*pb.Task
	fail    []*pb.Task
}

func NewTBNetwork() *TBNetwork {
	f, err := os.OpenFile(taskFolder, os.O_APPEND|os.O_WRONLY|os.O_CREATE, 0644)
	check(err)

	n := &TBNetwork{
		tasks: ReadTasksFromDisk(),
		lpnm:  &LamppostNetworkManager{},
		taskF: f,
	}
	return n
}

func (m *TBNetwork) Close() {
	m.taskF.Close()
}

func (m *TBNetwork) Update() {
	GetK8SConn().DiscoveryNetwork(m)
	m.lpnm.Update()
}

func ReadTasksFromDisk() map[string]*pb.Task {
	// |strlen|protolen|k|proto|
	// |4|4|strlen|protolen|
	tasks := make(map[string]*pb.Task)
	if _, err := os.Stat(taskFolder); os.IsNotExist(err) {
		return tasks
	}

	f, err := os.Open(taskFolder)
	check(err)
	defer f.Close()

	fi, err := f.Stat()
	check(err)

	fsize := fi.Size()
	fptr := int64(0)

	reader := bufio.NewReader(f)
	for fptr < fsize {
		_, err = f.Seek(fptr, 0)
		check(err)

		sbuf, err := reader.Peek(8)
		check(err)

		strlen := binary.BigEndian.Uint32(sbuf[:4])
		protolen := binary.BigEndian.Uint32(sbuf[4:])

		fptr += 8
		_, err = f.Seek(fptr, 0)
		check(err)

		cbuf, err := reader.Peek(int(strlen + protolen))
		check(err)

		key := string(cbuf[:strlen])
		pbuf := cbuf[strlen:]
		task := &pb.Task{}
		if err := proto.Unmarshal(pbuf, task); err != nil {
			log.Fatalln("Failed to parse address book:", err)
		}
		fptr += int64(strlen + protolen)
		tasks[key] = task
	}
	return tasks
}

func (m *TBNetwork) WriteTask(name string, task *pb.Task) bool {
	m.mu.Lock()
	defer m.mu.Unlock()
	if _, ok := m.tasks[name]; ok {
		return false
	}
	// First write to disk
	klen := len(name)
	out, err := proto.Marshal(task)
	plen := len(out)
	check(err)

	bs := make([]byte, 4)
	binary.BigEndian.PutUint32(bs, uint32(klen))
	_, err = m.taskF.Write(bs)
	check(err)
	binary.BigEndian.PutUint32(bs, uint32(plen))
	_, err = m.taskF.Write(bs)
	check(err)
	_, err = m.taskF.Write([]byte(name))
	check(err)
	_, err = m.taskF.Write(out)
	check(err)

	// Then add to memory
	m.tasks[name] = task

	// Assign Task to K8S
	GetK8SConn().SubmitTask(task)
	return true
}

func (m *TBNetwork) GetTaskByName(name string) *pb.Task {
	if t, ok := m.tasks[name]; ok {
		return t
	} else {
		return nil
	}
}

func (m *TBNetwork) GetNodes() []*pb.Node {
	var nodes []*pb.Node
	for _, n := range m.nodes {
		nodes = append(nodes, n)
	}
	return nodes
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
