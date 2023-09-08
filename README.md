# MARS 2.0
![](https://miro.medium.com/max/1041/0*9U14tXlY7nby0yTj)

Monitoring framework using prometheus, grafana and exporters. 
=======================

---

# What is MARS 2.0 made up of ?

## Server components:
These need to be running on any one host connected by network with the clients.

* Prometheus
* Grafana

## Client components 
Must be run on each machine that needs monitoring:
* node_exporter
* dcgm_exporter
* tp-link_exporter

# How to run

Before you begin, create an external docker network by running the following command on all clients and servers:
```
docker network create monitoring
```

Next, start the individual exporters on each client by executing:
```
<!-- Node Exporter (System metrics) -->
cd client\node_exporter
docker-compose up -d

<!-- DCGM Exporter (GPU) -->
cd client\dcgm_exporter
docker-compose up -d

<!-- TP Link Exporter (Power) -->
cd client\tp-link_exporter
docker-compose up -d
```

Then, on the server, configure Promtheus to scrape the metrics exposed above by changing the target to the client IP in `server\config\prometheus.yml`

Finally, start the server by running:
```
cd server
docker-compose up -d
```

## Viewing
Aggregated Prometheus data can be viewed on the server as raw data at `http://prometheus:9090` or visually via Grafana at `http://localhost:3000` 

## Troubleshooting
### File permission errors on server (grafana)
If you encounter file permission errors when running grafana, check that the `server/.env` file has the correct `USER_ID` and `GROUP_ID`.
You can check your user and group ID in Linux with the `id` command. 
