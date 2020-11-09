package main

import (
	"google.golang.org/grpc"
)

func NewGrpcConn(address string) (*grpc.ClientConn, error) {
	conn, err := grpc.Dial(address, grpc.WithInsecure(), grpc.WithBlock())
	if err != nil {
		return nil, err
	}
	return conn, nil
}
