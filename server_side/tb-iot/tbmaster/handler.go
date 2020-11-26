package main

import (
	"context"
	"errors"
	"fmt"
	"regexp"

	pb "github.com/KitFung/tb-iot/proto"
)

type CliHandler interface {
}

type TestBedCliServer struct {
	pb.UnimplementedTestBedMasterServer
	tbnet *TBNetwork
}

var nameRegex = regexp.MustCompile("^[a-z0-9]([-a-z0-9]*[a-z0-9])?$")

func VerifyTaskName(name string) bool {
	return nameRegex.MatchString(name)
}

func (m *TestBedCliServer) NewTask(ctx context.Context, in *pb.NewTaskRequest) (*pb.NewTaskResponse, error) {
	fmt.Println("Receive NewTask")
	// Step 1. Verifiy Input
	if !VerifyTaskName(*in.TaskName) {
		reason := "Unacceptable task name"
		return &pb.NewTaskResponse{
			Error: []string{reason},
		}, errors.New(reason)
	}
	fmt.Println("Add NewTask")
	// Step 2: Add the task
	fmt.Printf("%v\n", in.TaskInfo)
	succ := m.tbnet.WriteTask(*in.TaskName, in.TaskInfo)
	if succ {
		fmt.Println("Succ")
		resp := &pb.NewTaskResponse{}
		return resp, nil
	} else {
		fmt.Println("Failed")
		reason := "Repeated Task"
		return &pb.NewTaskResponse{
			Error: []string{reason},
		}, errors.New(reason)
	}
}

func (m *TestBedCliServer) GetTask(ctx context.Context, in *pb.GetTaskRequest) (*pb.GetTaskResponse, error) {
	if in.TaskId != nil {
	} else if in.TaskName != nil {
		t := m.tbnet.GetTaskByName(in.GetTaskName())
		if t != nil {
			return &pb.GetTaskResponse{
				TaskInfo: t,
			}, nil
		} else {
			reason := "No existing task"
			return &pb.GetTaskResponse{
				Error: []string{reason},
			}, errors.New(reason)
		}
	}
	return nil, nil
}

func (m *TestBedCliServer) ListNodes(ctx context.Context, in *pb.ListNodesRequest) (*pb.ListNodesResponse, error) {
	return &pb.ListNodesResponse{
		Network: &pb.NodeNetwork{
			Node: m.tbnet.GetNodes(),
		},
	}, nil
}
