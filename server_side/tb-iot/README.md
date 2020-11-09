```
go get google.golang.org/grpc/cmd/protoc-gen-go
go get google.golang.org/grpc/cmd/protoc-gen-go-grpc
go install google.golang.org/grpc/cmd/protoc-gen-go
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc


```
protoc --proto_path=proto --go_out=proto \
    --go_opt=paths=source_relative \
    --go-grpc_out=proto --go-grpc_opt=paths=source_relative \
    proto/tb.proto
```
