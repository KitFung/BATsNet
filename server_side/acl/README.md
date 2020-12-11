## Flow
---

```
User submit task -> tbmaster validate task -> tbmaster submit to k8s (Preserve right in acl) -> pod alloacte to node -> task running on node -> access controler (Check right in acl)
```

## Required change to the system
---

1. This acl component
2. The controller need a function to check acl
3. Register acl in tbmaster
4. a proxy deamon set for fog nodes that help controller access acl
5. Pass pod name and pod id to task as environment variable
