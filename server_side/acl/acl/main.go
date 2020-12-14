package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"

	"github.com/gorilla/mux"
)

type ServieRequest struct {
	ServiceName string
	PodName     string
	PodID       string
}

func service(w http.ResponseWriter, r *http.Request) {
	decoder := json.NewDecoder(r.Body)
	w.Header().Set("Content-Type", "application/json")
	var service ServieRequest
	if err := decoder.Decode(&service); err != nil {
		w.Header().Set("Content-Type", "application/json; charset=UTF-8")
		w.WriteHeader(422) // unprocessable entity
		err = json.NewEncoder(w).Encode(err)
		check(err)
	}
	fmt.Printf("Receive Request for %+v\n", service)
	success := GetAclDB().MonopolizeService(service.ServiceName, service.PodName, service.PodID)
	w.Header().Set("Content-Type", "application/json; charset=UTF-8")
	if success {
		w.WriteHeader(http.StatusAccepted)
	} else {
		w.WriteHeader(http.StatusForbidden)
	}
}

func main() {
	r := mux.NewRouter()
	r.HandleFunc("/", service).Methods(http.MethodPost)
	log.Fatal(http.ListenAndServe(":8080", r))
}
