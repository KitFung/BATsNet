package main

import (
	"database/sql"

	_ "github.com/mattn/go-sqlite3"

	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/rest"
)

const kDB_TYPE = "sqlite3"
const kDefault_DB_PATH = "/db/acl.db"
const TBAppsNamespace = "testbed-apps"

type AclDB struct {
	db        *sql.DB
	k8sClient *kubernetes.Clientset
}

func NewAclDb() *AclDB {
	db, err := sql.Open(kDB_TYPE, kDefault_DB_PATH)
	check(err)

	acl := &AclDB{}
	acl.db = db

	config, err := rest.InClusterConfig()
	check(err)
	clientSet, err := kubernetes.NewForConfig(config)
	check(err)
	acl.k8sClient = clientSet

	return acl
}

func (m *AclDB) InitialDB() {
	stmt := `
	CREATE TABLE IF NOT EXISTS acl (
		id integer not null primary key,
		serviceName varchar(64) NOT NULL UNIQUE,
		podName varchar(64) NOT NULL,
		podID text NOT NULL);
	CREATE UNIQUE INDEX IF NOT EXISTS acl_service_name 
		ON acl (serviceName);
	`
	_, err := m.db.Exec(stmt)
	check(err)
}

func (m *AclDB) CheckPodAlive(podName string, podID string) bool {
	pod, err := m.k8sClient.CoreV1().Pods(TBAppsNamespace).Get(podName, metav1.GetOptions{})
	check(err)
	return string(pod.GetUID()) == podID
}

func (m *AclDB) MonopolizeService(serviceName string, podName string, podID string) bool {
	stmt, err := m.db.Prepare("SELECT podName, podID FROM acl where service = ?")
	check(err)
	var oldPodName string
	var oldPodID string
	err = stmt.QueryRow(serviceName).Scan(&oldPodName, &oldPodID)
	if err == sql.ErrNoRows {
		// INSERT
		stmt, err = m.db.Prepare("INSERT INTO acl(service, podName, podID) VALUES(?, ?, ?)")
		check(err)
		_, err = stmt.Exec(serviceName, podName, podID)
		check(err)
		return true
	}
	check(err)
	if m.CheckPodAlive(oldPodName, oldPodID) {
		return false
	} else {
		// UPDATE
		stmt, err = m.db.Prepare("UPDATE acl set podName=?, podID=? where service=?")
		check(err)
		_, err = stmt.Exec(podName, podID, serviceName)
		check(err)
		return true
	}
}
