package main

import (
	"context"

	pb "github.com/KitFung/tb-iot/proto"
)

type CliHandler interface {
}

type TestBedCliServer struct {
	pb.UnimplementedTestBedMasterServer
}

func (m *TestBedCliServer) NewTask(ctx context.Context, in *pb.NewTaskRequest) (*pb.NewTaskResponse, error) {
	return nil, nil
}

func (m *TestBedCliServer) GetTask(ctx context.Context, in *pb.GetTaskRequest) (*pb.GetTaskResponse, error) {
	return nil, nil
}

func (m *TestBedCliServer) ListNodes(ctx context.Context, in *pb.ListNodesRequest) (*pb.ListNodesResponse, error) {
	return nil, nil
}
