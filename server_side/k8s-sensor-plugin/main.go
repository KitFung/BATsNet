package main

import (
	"flag"
	"os"
	"syscall"

	"github.com/fsnotify/fsnotify"
	log "github.com/sirupsen/logrus"
	pluginapi "k8s.io/kubelet/pkg/apis/deviceplugin/v1beta1"
)

var failOnInitErrorFlag = flag.Bool(
	"fail-on-init-error",
	true,
	"fail the plugin if an error is encountered during initialization, otherwise block indefinitely [default: true]\n")

var flagLogLevel = flag.String("log-level", "info", "Define the logging level: error, info, debug.")

func main() {
	flag.Parse()
	switch *flagLogLevel {
	case "debug":
		log.SetLevel(log.DebugLevel)
	case "info":
		log.SetLevel(log.InfoLevel)
	}
	watcher, err := newFSWatcher(pluginapi.DevicePluginPath)
	if err != nil {
		log.Printf("Failed to created FS watcher: %s.", err)
		os.Exit(1)
	}
	defer watcher.Close()

	log.Println("Starting OS watcher.")
	sigs := newOSWatcher(syscall.SIGHUP, syscall.SIGINT, syscall.SIGTERM, syscall.SIGQUIT)

	var plugins []*SensorPlugin

restart:
	for _, p := range plugins {
		p.Stop()
	}

	log.Println("Retreiving plugins.")
	plugins = GetPlugins()

	for _, p := range plugins {
		if err := p.Start(); err != nil {
			log.SetOutput(os.Stderr)
			log.Println("Could not contact Kubelet, retrying. Did you enable the device plugin feature gate?")
			log.Printf("You can check the prerequisites at: https://github.com/NVIDIA/k8s-device-plugin#prerequisites")
			log.Printf("You can learn how to set the runtime at: https://github.com/NVIDIA/k8s-device-plugin#quick-start")
			goto restart
		}
	}

L:
	for {

		select {

		case event := <-watcher.Events:
			if event.Name == pluginapi.KubeletSocket && event.Op&fsnotify.Create == fsnotify.Create {
				log.Printf("inotify: %s created, restarting.", pluginapi.KubeletSocket)
				goto restart
			}

		case err := <-watcher.Errors:
			log.Printf("inotify: %s", err)

		case s := <-sigs:
			switch s {
			case syscall.SIGHUP:
				log.Println("Received SIGHUP, restarting.")
				goto restart
			default:
				log.Printf("Received signal \"%v\", shutting down.", s)
				break L
			}
		}
	}
}
