# speed_test

Sample benchmark code for comparing IO multiplexing methods

## Usage

Server:

```
Usage: ./udp_msg_server [ OPTIONS ]
Options:
[-p, --port LISTEN_PORT]
[-S, --scheduler] Scheduler Algorithm (OTHER, BATCH, IDLE, FIFO, RR)
[-P, --priority NUMBER] Realtime priority between 0.99 for realtime schedulers
[-r, --receive-buffer-size SIZE]
[-m, --method SELECT|EPOLL|THREAD]

Scheduler Types can be: OTHER (default), BATCH, IDLE, FIFO, RR
```

Client:

```
Usage: ./udp_msg_client [ OPTIONS ]
Options:
[-s, --server SERVER_IP] Remote Server Ip
[-p, --port PORT] Remote Server Port
[-m, --msg-payload PAYLOAD] Between 0..1500
[-t, --total TOTAL_SEND] Number of packets
[-S, --scheduler] Scheduler Algorithm (OTHER, BATCH, IDLE, FIFO, RR)
[-P, --priority NUMBER] Realtime priority between 0.99 for realtime schedulers
[-c] Use connected udp mode
[-h, --help]
```

## Benchmark - 1

Server:

- Ethernet: Broadcom Corporation NetXtreme BCM57766 Gigabit Ethernet PCIe
- CPU: Intel i5-3330S CPU @ 2.70GHz Quad Core

Client:

- Ethernet: Broadcom Corporation NetXtreme BCM5721 Gigabit Ethernet PCI Express
- CPU: Intel Xeon CPU 3040 @ 1.86GHz

There are 4 gigabit switch between client and server.
1 million packets sent for each test.

Results:

| Client Scheduler | Connected Mode | Payload | Server Scheduler | RcvBuf | Loss %| Data       | Wire       |
|:----------------:|:--------------:|:-------:|:----------------:|:------:|:-----:|:----------:|:----------:|
| SCHED_OTHER      | No             | 64      | SCHED_OTHER      | 256Kb  | 1.272 | 231.6 Mbps | 383.5 Mbps |
| SCHED_OTHER      | Yes            | 64      | SCHED_OTHER      | 256Kb  | 1.320 | 246.1 Mbps | 407.7 Mbps |
| SCHED_FIFO       | No             | 64      | SCHED_OTHER      | 256Kb  | 0.799 | 245.7 Mbps | 407.0 Mbps |
| SCHED_FIFO       | Yes            | 64      | SCHED_OTHER      | 256Kb  | 0.756 | 246.2 Mbps | 407.8 Mbps |
| SCHED_FIFO       | No             | 64      | SCHED_OTHER      | 2 Mb   | 0.613 | 250.8 Mbps | 415.4 Mbps |
| SCHED_FIFO       | No             | 64      | SCHED_OTHER      | 4 Mb   | 0.005 | 252.3 Mbps | 417.9 Mbps |
| SCHED_FIFO       | No             | 64      | SCHED_OTHER      | 8 Mb   | 0     | 252.9 Mbps | 418.9 Mbps |
| SCHED_FIFO       | No             | 64      | SCHED_OTHER      | 16 Mb  | 0     | 251.9 Mbps | 417.2 Mbps |
| SCHED_FIFO       | No             | 1500    | SCHED_OTHER      | 16 Mb  | 0     | 858.2 Mbps | 882.2 Mbps |
| SCHED_FIFO       | Yes            | 1500    | SCHED_OTHER      | 16 Mb  | 0     | 856.0 Mbps | 879.2 Mbps |
| SCHED_FIFO       | Yes            | 1472    | SCHED_OTHER      | 16 Mb  | 0     | 933.9 Mbps | 969.5 Mbps |
| SCHED_FIFO       | Yes            | 1472    | SCHED_RR         | 16 Mb  | 0     | 934.9 Mbps | 961.6 Mbps |



