package main

import (
	"encoding/json"
	"io"
	"io/ioutil"
	"log"
	"net/http"

	"github.com/gorilla/mux"
)

type ServieRequest struct {
	serviceName string
	podName     string
	podID       string
}

func service(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	var service ServieRequest
	body, err := ioutil.ReadAll(io.LimitReader(r.Body, 1048576))
	check(err)
	err = r.Body.Close()
	check(err)
	if err = json.Unmarshal(body, &service); err != nil {
		w.Header().Set("Content-Type", "application/json; charset=UTF-8")
		w.WriteHeader(422) // unprocessable entity
		err = json.NewEncoder(w).Encode(err)
		check(err)
	}

	success := GetAclDB().MonopolizeService(service.serviceName, service.podName, service.podID)
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
	log.Fatal(http.ListenAndServe(":8080", nil))
}
