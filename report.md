# CS 425 Midterm Project (X1): Network Performance Report

**Student:** Harrison Jansen van Beek  
**Course:** CS 425 Computer Networks  
**Date:** Fall 2026  
**Repository:** `git@github.com:harrisonjansenva/cs415-x1.git`  
**Commits Measured:** `337016c` (Laptop, Onyx), `2ee226e` (Codespace)

---

# Task 3: Predictions

*Original predictions recorded in report.md before running measurements:*

## Predictions
 the great circle distance there and back at the speed of light in fiber, 2 x 10^8 m/s (200 km per millisecond).
* CDN: ~1ms. I'm guessing there has to be one very close to here (within 200 km)
* SLC, UT: 466.3 NM, so I'm guessing about 2.3 ms.
* Berkeley, CA: 812 km, so about 4 ms.
* Rochester, NY: 3,089.6 km, so about 15.5 ms.
* Erlangen, Germany: 8,471 km, so about 42.5 ms.
* Nomi, Japan: 8488 km, so about the same amount of time (42.5 ms).
* I'm guessing getting to either Germany or Japan will take the longest, depending on whether we are on an east or west coast server. If East, Japan, if West, Germany.
* All three vantage points will be behind a NAT since I am on a wifi network, the campus routing through a VPN, and Codespaces I'm sure does not expose the address we are given to the public internet.
* I think after we have caached the DNS resolution, it will be significantly faster for each successive lookup.
* I think they will take approximately the same time, if slightly longer on the file from Japan.

---

# Task 5: Analysis

## Chapter 1: Delay and Throughput

### 1. Distance versus Delay

The table below reports for every mirror target across all three vantage points:
- Great-circle distance ($d$ in km)
- Theoretical propagation round-trip time ($\text{RTT}_{\text{prop}} = \frac{2d}{c_{\text{fiber}}}$ where $c_{\text{fiber}} = 2 \times 10^8\text{ m/s} = 200\text{ km/ms} = \frac{d}{100\text{ km/ms}}$)
- Measured minimum connect time ($\text{Min Connect}$ in ms)
- The ratio ($\text{Min Connect} / \text{RTT}_{\text{prop}}$)

#### Vantage Point 1: Laptop (Boise, ID — 43.6330° N, 116.2951° W)

| Mirror | Great Circle Distance | Propagation RTT ($\text{RTT}_{\text{prop}}$) | Min Connect Time | Ratio ($\text{Connect} / \text{Prop}$) |
| :--- | :--- | :--- | :--- | :--- |
| **Utah** (`mirrors.xmission.com`) | 476.2 km | 4.76 ms | 43.4 ms | **9.11** |
| **California** (`mirrors.ocf.berkeley.edu`) | 817.7 km | 8.18 ms | 45.4 ms | **5.55** |
| **New York** (`mirrors.rit.edu`) | 3,090.7 km | 30.91 ms | 91.2 ms | **2.95** |
| **Germany** (`ftp.fau.de`) | 8,454.0 km | 84.54 ms | 179.6 ms | **2.12** |
| **Japan** (`ftp.jaist.ac.jp`) | 8,480.2 km | 84.80 ms | 160.5 ms | **1.89** |

#### Vantage Point 2: Onyx (Boise State Campus — 43.6029° N, 116.1999° W)

| Mirror | Great Circle Distance | Propagation RTT ($\text{RTT}_{\text{prop}}$) | Min Connect Time | Ratio ($\text{Connect} / \text{Prop}$) |
| :--- | :--- | :--- | :--- | :--- |
| **Utah** (`mirrors.xmission.com`) | 475.2 km | 4.75 ms | 49.8 ms | **10.48** |
| **California** (`mirrors.ocf.berkeley.edu`) | 816.8 km | 8.17 ms | 25.0 ms | **3.06** |
| **New York** (`mirrors.rit.edu`) | 3,090.8 km | 30.91 ms | 52.5 ms | **1.70** |
| **Germany** (`ftp.fau.de`) | 8,455.0 km | 84.55 ms | 145.2 ms | **1.72** |
| **Japan** (`ftp.jaist.ac.jp`) | 8,481.2 km | 84.81 ms | 199.1 ms | **2.35** |

#### Vantage Point 3: Codespace (Azure West US, Moses Lake, WA — 47.1301° N, 119.2781° W)

| Mirror | Great Circle Distance | Propagation RTT ($\text{RTT}_{\text{prop}}$) | Min Connect Time | Ratio ($\text{Connect} / \text{Prop}$) |
| :--- | :--- | :--- | :--- | :--- |
| **Utah** (`mirrors.xmission.com`) | 921.8 km | 9.22 ms | 31.1 ms | **3.37** |
| **California** (`mirrors.ocf.berkeley.edu`) | 1,058.2 km | 10.58 ms | 42.5 ms | **4.02** |
| **New York** (`mirrors.rit.edu`) | 3,258.5 km | 32.59 ms | 71.5 ms | **2.19** |
| **Germany** (`ftp.fau.de`) | 8,246.6 km | 82.47 ms | 148.6 ms | **1.80** |
| **Japan** (`ftp.jaist.ac.jp`) | 8,054.9 km | 80.55 ms | 108.2 ms | **1.34** |

---

### 2. Where the Extra Time Goes

Every measured ratio in Question 1 is strictly greater than 1.0 (ranging from $1.34$ to $10.48$). As explained in **Kurose & Ross Section 1.4 (Delay, Loss, and Throughput in Packet-Switched Networks)**, total nodal delay consists of four fundamental components:
$$d_{\text{nodal}} = d_{\text{proc}} + d_{\text{queue}} + d_{\text{trans}} + d_{\text{prop}}$$

The theoretical baseline calculation only accounts for straight-line propagation delay ($d_{\text{prop}}$) through a perfect chord/arc. In real-world physical networks:
1. **Circuitous Physical Fiber Routing:** Fiber optic cables are laid along existing highway rights-of-way, railways, and undersea submarine cable paths. Fiber paths often incur a "detour factor" of 1.3× to 2.1× longer than the direct great-circle distance.
2. **Nodal Processing Delay ($d_{\text{proc}}$):** Every intermediate router (often 12–25 hops) must examine the IP header, check the checksum, and perform longest-prefix matching in its forwarding table.
3. **Queuing Delay ($d_{\text{queue}}$):** Packets sit in output port buffers waiting for link transmission behind cross-traffic.
4. **Transmission Delay ($d_{\text{trans}}$):** The time required to push packet bits onto each physical link across each hop.
5. **Last-Mile Access Overheads:** Wi-Fi framing, contention, carrier sensing (CSMA/CA), and modem/NAT processing introduce substantial fixed latency (often 5–20 ms) before the packet even reaches the Internet backbone.

#### Dominant Components: Japan vs. Utah
- **For Japan (`ftp.jaist.ac.jp`):** **Propagation delay ($d_{\text{prop}}$) heavily dominates.** Traveling ~8,500 km across the continent and the Pacific Ocean imposes an unavoidable speed-of-light propagation delay of at least ~80–85 ms each way, creating a base round-trip floor of ~160–170 ms. Processing and transmission delays represent only a minor fraction of the total connect time (ratio 1.34–2.35).
- **For Utah (`mirrors.xmission.com`):** **Queuing delay ($d_{\text{queue}}$), processing delay ($d_{\text{proc}}$), and last-mile access delays dominate.** Because Utah is physically close (~476 km), the speed-of-light propagation RTT is only $4.76\text{ ms}$. However, the measured connect time is $31.1\text{–}49.8\text{ ms}$ (a ratio of 7× to 10×). The vast majority of the elapsed connect time is spent traversing local access networks, Wi-Fi contention, ISP peering point router queues, and server TCP stack scheduling rather than fiber transit.

---

### 3. Minimum, Not Mean

Question 1 specifies using the **minimum** connect time rather than the mean because the goal of physical network modeling is to measure the deterministic, physical propagation floor of the path.

As detailed in Kurose & Ross Section 1.4.2, queuing delay is inherently stochastic; it fluctuates dynamically from millisecond to millisecond depending on buffer occupancy and instantaneous traffic bursts ($d_{\text{queue}} \rightarrow \infty$ as traffic intensity $I \rightarrow 1$). Similarly, client OS scheduling and wireless channel contention add variable jitter.

Looking at the spread across our 5 trials:
- **Laptop to New York (`mirrors.rit.edu`):**
  - Trial 1: $127.3\text{ ms}$
  - Trial 2: $95.1\text{ ms}$
  - Trial 3: $91.2\text{ ms}$ (Minimum)
  - Trial 4: $138.3\text{ ms}$ (Spike of +47.1 ms!)
  - Trial 5: $93.7\text{ ms}$
  - Mean = $109.1\text{ ms}$, whereas Min = $91.2\text{ ms}$.
- **Codespace to Japan (`ftp.jaist.ac.jp`):**
  - Trial 1: $130.5\text{ ms}$
  - Trial 2: $108.2\text{ ms}$ (Minimum)
  - Trial 3: $130.4\text{ ms}$
  - Trial 4: $130.6\text{ ms}$
  - Trial 5: $130.5\text{ ms}$
  - The minimum ($108.2\text{ ms}$) captured an exceptionally clean, queue-free transit, whereas the other trials experienced an extra ~22 ms of queuing/buffering.

The minimum connect time captures a trial that experienced near-zero queuing delay, closest to the true physical latency of the route. Taking the mean would contaminate the physical distance estimate with random queuing noise.

---

### 4. Transmission Delay

A TCP `SYN` segment is 64 bytes on the wire ($L = 64 \text{ bytes} \times 8 \text{ bits/byte} = 512 \text{ bits}$).  
According to Kurose & Ross Section 1.4.1, transmission delay is:
$$d_{\text{trans}} = \frac{L}{R}$$

#### Calculations
1. **On a 10 Mbit/s link ($R = 10 \times 10^6 \text{ bits/s}$):**
   $$d_{\text{trans}} = \frac{512 \text{ bits}}{10 \times 10^6 \text{ bits/s}} = 0.0000512 \text{ s} = \mathbf{0.0512 \text{ ms}} \quad (51.2 \text{ }\mu\text{s})$$
2. **On a 1 Gbit/s link ($R = 1 \times 10^9 \text{ bits/s}$):**
   $$d_{\text{trans}} = \frac{512 \text{ bits}}{1 \times 10^9 \text{ bits/s}} = 0.000000512 \text{ s} = \mathbf{0.000512 \text{ ms}} \quad (0.512 \text{ }\mu\text{s})$$

#### Comparison to Utah Propagation Delay
From our laptop in Boise, the one-way propagation delay to Utah is $d_{\text{prop}} \approx 2.38\text{ ms}$ (round-trip $\approx 4.76\text{ ms}$).
- On a 10 Mbit/s link, the transmission delay ($0.0512\text{ ms}$) is only **2.1%** of the one-way propagation delay.
- On a 1 Gbit/s link, the transmission delay ($0.000512\text{ ms}$) is only **0.02%** of the one-way propagation delay (propagation delay is 4,650× larger!).

#### Takeaway for Small Messages
For small control packets (such as TCP SYNs, ACKs, FINs, and DNS queries), **transmission delay is completely negligible**. Performance is entirely governed by **propagation delay** and intermediate router queuing delay. High transmission bandwidth (e.g., upgrading from 10 Mbps to 1 Gbps) does virtually nothing to speed up latency-sensitive handshakes.

---

### 5. Throughput

Across our three vantage points, downloading the ~13.3 MB `Packages.gz` file yielded:

| Vantage Point | Target | Trial 1 Throughput | Trial 2 Throughput | Trial 3 Throughput | Avg Transfer Time |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Laptop** | Utah | 168.85 Mbit/s | 141.27 Mbit/s | 138.10 Mbit/s | **0.72 s** |
| **Laptop** | Japan | 10.17 Mbit/s | 5.08 Mbit/s | 9.69 Mbit/s | **14.16 s** |
| **Onyx** | Utah | 89.29 Mbit/s | 92.16 Mbit/s | 92.31 Mbit/s | **1.17 s** |
| **Onyx** | Japan | 0.90 Mbit/s | 1.63 Mbit/s | 0.96 Mbit/s | **98.71 s** |
| **Codespace** | Utah | 190.19 Mbit/s | 187.59 Mbit/s | 312.75 Mbit/s | **0.49 s** |
| **Codespace** | Japan | 21.31 Mbit/s | 17.49 Mbit/s | 25.69 Mbit/s | **5.08 s** |

#### Is the access link the bottleneck in both cases? How do you know?
**No, the access link is NOT the bottleneck in both cases.**

**Evidence:**  
If the local access link were the bottleneck for both downloads, the throughput would hit the same performance ceiling regardless of the remote server's geographic location.
- On the **Laptop**, the Utah download reached **168.85 Mbit/s**, proving that the local Wi-Fi and ISP access link have at least 168 Mbit/s of available downlink capacity. Yet over that exact same access link, the Japan download crawled at **5.08 to 10.17 Mbit/s** (a 16× to 33× drop!).
- On **Onyx**, Utah consistently transferred at **~92 Mbit/s**, while Japan crawled at **0.90 to 1.63 Mbit/s** (taking up to 119 seconds!).
- On **Codespace**, the Azure datacenter pipe sustained **312.75 Mbit/s** from Utah, but only **17.49 to 25.69 Mbit/s** from Japan.

For the Japan transfer, the bottleneck is the high end-to-end round-trip time ($\text{RTT} \approx 160\text{–}200\text{ ms}$), the Bandwidth-Delay Product (BDP), and TCP congestion control dynamics (window growth clamping and high-RTT packet loss recovery), rather than the local access link.

---

## Chapter 2: The Application Layer

### 6. DNS Caching

Examining the DNS lookup times across 5 trials for Germany (`ftp.fau.de`):

| Vantage Point | Trial 1 DNS | Trial 2 DNS | Trial 3 DNS | Trial 4 DNS | Trial 5 DNS |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Laptop** | **211.8 ms** | 2.9 ms | 3.4 ms | 3.7 ms | 3.0 ms |
| **Codespace** | **3.8 ms** | 0.5 ms | 0.5 ms | 0.4 ms | 0.5 ms |
| **Onyx** | **28.9 ms** | 1029.4 ms* | 32.0 ms | 179.0 ms | 32.1 ms |

*(On Onyx, campus recursive resolver queries to outside nameservers experienced intermittent queue timeouts/retries).*

#### Explanation (Kurose & Ross Section 2.4.2)
- **Trial 1 (Cold Cache / Cache Miss):** The local recursive DNS server (or laptop stub resolver) did not have an active cache entry for `ftp.fau.de`. The resolver had to perform an iterative resolution walk: contacting a Root Server, receiving a referral to the `.de` ccTLD servers, querying a `.de` nameserver in Germany, receiving a referral to FAU's authoritative nameservers, and finally querying `ns1.fau.de` in Erlangen to retrieve the A record. This required multiple transatlantic round trips, taking **211.8 ms** on the laptop.
- **Trials 2 through 5 (Warm Cache / Cache Hit):** Upon receiving the DNS reply in Trial 1, the local recursive DNS server stored the hostname-to-IP mapping in its local cache along with the record's Time-To-Live (TTL). When Trials 2 through 5 queried the resolver, the answer was returned immediately from local DRAM over the LAN, dropping lookup time to **~3 ms** on the laptop (a **70× speedup**) and **~0.5 ms** in Codespace.

---

### 7. The DNS Hierarchy

Using the DNS trace from our **Laptop** (`measurements-laptop.txt`), where `dig +trace ftp.jaist.ac.jp` executed successfully:

```text
; <<>> DiG 9.10.6 <<>> +trace ftp.jaist.ac.jp
;; Received 1128 bytes from 192.168.4.1#53(192.168.4.1) in 31 ms
;; Received 468 bytes from 199.7.83.42#53(l.root-servers.net) in 45 ms
```

#### Ordered Server Query Progression and Hierarchy Mapping (Section 2.4.1):
1. **Local DNS Resolver (`192.168.4.1#53`):**
   - **Role:** Local DNS Server / Default Gateway.
   - **Answer:** Returned a referral containing the NS records for the 13 Root DNS Servers (`a.root-servers.net` through `m.root-servers.net`).
2. **Root DNS Name Server (`l.root-servers.net` / `199.7.83.42#53`):**
   - **Role:** Root Name Server (top tier of the DNS hierarchy).
   - **Answer:** A referral (delegation) containing NS records and glue A/AAAA records for the `.jp` country-code Top-Level Domain (ccTLD) name servers (`a.dns.jp` through `g.dns.jp`).
3. **TLD Name Server (`a.dns.jp`):**
   - **Role:** Top-Level Domain (TLD) Name Server for the `.jp` ccTLD.
   - **Answer:** A referral containing the authoritative NS records for the `jaist.ac.jp` academic second-level domain (`dns1.jaist.ac.jp`, `dns2.jaist.ac.jp`).
4. **Authoritative Name Server (`dns1.jaist.ac.jp` / `150.65.1.1`):**
   - **Role:** Authoritative Name Server for the Japan Advanced Institute of Science and Technology (`jaist.ac.jp`).
   - **Answer:** The final, authoritative Type A resource record mapping `ftp.jaist.ac.jp` directly to IP address **`150.65.7.130`**.

---

### 8. The CDN

#### IP Address Resolution Across Vantage Points
- **Laptop:** `104.20.23.154` (Min Connect: **26.5 ms**)
- **Onyx:** `104.20.23.154` and `172.66.147.243` (Min Connect: **11.6 ms**)
- **Codespace:** `104.20.23.154` and `172.66.147.243` (Min Connect: **4.9 ms**)

#### Connect Time Comparison with Mirrors
The CDN (`example.com`) achieved drastically lower connect times than any regional mirror:
- At **Codespace**, connect time was **4.9 ms** (vs Utah 31.1 ms, CA 42.5 ms, NY 71.5 ms, Germany 148.6 ms, Japan 108.2 ms).
- At **Onyx**, connect time was **11.6 ms** (vs Utah 49.8 ms, CA 25.0 ms, NY 52.5 ms, Germany 145.2 ms, Japan 199.1 ms).

#### Technique and Mechanism (Kurose & Ross Section 2.5)
The evidence conclusively points to **BGP Anycast routing** combined with edge server deployment ("Enter Deep").

The exact same IPv4 addresses (`104.20.23.154` and `172.66.147.243`, owned by Cloudflare) are advertised via BGP from hundreds of edge data centers globally. When a client sends a SYN packet to `104.20.23.154`:
- Routers in Washington (Codespace) route along the shortest BGP path to Cloudflare's Seattle/Quincy edge POP, connecting in **4.9 ms**.
- Routers in Boise (Onyx) route via Boise State's direct regional exchange peering to the nearest Cloudflare point of presence, connecting in **11.6 ms**.
- The client does not travel to a centralized origin server; BGP Anycast automatically delivers packets to the topologically closest edge node.

---

### 9. Counting Round Trips

#### Estimating Server Processing Time ($T_{\text{server}}$)
In HTTP/1.1 over an established TCP connection, the time to first byte is:
$$\text{First Byte Time} \approx 1\text{ RTT} + T_{\text{server}}$$
Since the TCP connect time is approximately $1\text{ RTT}$, we estimate:
$$T_{\text{server}} \approx \text{First Byte Time} - \text{Connect Time}$$

1. **Utah (`mirrors.xmission.com`) on Onyx:**
   - Connect Time: $49.8\text{ ms}$
   - First Byte Time: $50.3\text{ ms}$
   - $$T_{\text{server}} \approx 50.3\text{ ms} - 49.8\text{ ms} = \mathbf{0.5\text{ ms}}$$
2. **Utah (`mirrors.xmission.com`) on Codespace:**
   - Connect Time: $37.6\text{ ms}$
   - First Byte Time: $38.3\text{ ms}$
   - $$T_{\text{server}} \approx 38.3\text{ ms} - 37.6\text{ ms} = \mathbf{0.7\text{ ms}}$$
3. **Japan (`ftp.jaist.ac.jp`) on Laptop:**
   - Connect Time: $165.4\text{ ms}$
   - First Byte Time: $171.9\text{ ms}$
   - $$T_{\text{server}} \approx 171.9\text{ ms} - 165.4\text{ ms} = \mathbf{6.5\text{ ms}}$$

#### Round Trips from DNS Lookup to Last Byte of README
1. **DNS Lookup:** 1 RTT (to local resolver, or multiple if uncached).
2. **TCP 3-Way Handshake:** 1 RTT (`SYN` $\rightarrow$, `SYN-ACK` $\leftarrow$).
3. **HTTP Request and File Transfer:** 1 RTT (`GET` $\rightarrow$, `HTTP/1.1 200 OK` + body $\leftarrow$).
   - The `README` file is only 1,198 bytes, and total response with headers is ~1,500 bytes. Because this fits within the initial congestion window and maximum segment size, the server sends the entire file in the very first window of data segments immediately following the GET.
- **Total Round Trips:** **3 RTTs** for cold DNS, or **2 RTTs** for warm DNS.

#### Comparison with Section 2.2 Non-Persistent HTTP
In Kurose & Ross Section 2.2, non-persistent HTTP response time is modeled as:
$$T_{\text{response}} = 2 \times \text{RTT} + T_{\text{transmit}}$$
where one RTT is spent establishing the TCP connection and one RTT is spent requesting and receiving the small HTML/object file. Our empirical results match the textbook formula.

---

## Chapter 3: The Transport Layer

### 10. The Handshake

#### Segments Sent and Received When `connect()` Returns
During the standard TCP three-way handshake:
1. Client sends `SYN` (Segment 1).
2. Server responds with `SYN-ACK` (Segment 2).
3. Client receives `SYN-ACK`.
4. Client kernel transmits `ACK` (Segment 3) back to the server.

When the POSIX `connect()` system call returns on the client, exactly **two segments have traversed the network**:
- `SYN` has been sent.
- `SYN-ACK` has been received.

#### Why the Client Does Not Wait for the Third Segment
The third segment (`ACK`) is transmitted **by the client itself**. A host never needs to block waiting on a segment that it is sending. As soon as the client receives the server's `SYN-ACK`, the client has verified both directions of communication:
- Its own transmission reached the server (proven by the server's ACK).
- The server's transmission reached the client (proven by receiving the SYN-ACK).

At this exact microsecond, the client OS network stack marks the TCP state as `ESTABLISHED` and unblocks `connect()`. The third segment (`ACK`) is dispatched asynchronously by the kernel, typically piggybacked directly with the client's first outgoing payload segment (the `HTTP/1.1 GET` request). Therefore, `connect()` takes exactly **1 RTT**, not 1.5 RTT.

---

### 11. Demultiplexing

From the **"two at once"** section of `measurements-laptop.txt`, where two concurrent instances of `pageload` ran against the Utah mirror (`198.60.22.13:80`):

- **Instance 1 4-tuple:** `192.168.4.83:53687 -> 198.60.22.13:80`
- **Instance 2 4-tuple:** `192.168.4.83:53686 -> 198.60.22.13:80`

#### What is Different?
The **source port number** (`53687` vs `53686`).

#### Who Picked It?
The **client operating system's kernel** (transport layer network stack). Because `pageload` did not call `bind()` explicitly before `connect()`, the OS automatically assigned an unused ephemeral port number from its dynamic port range (`net.inet.ip.portrange` on macOS, `ip_local_port_range` on Linux).

#### How the Server Tells Connections Apart (Section 3.2)
According to Kurose & Ross Section 3.2 (Connection-Oriented Demultiplexing), TCP demultiplexing is governed by the full **4-tuple**:
$$(\text{Source IP}, \text{Source Port}, \text{Destination IP}, \text{Destination Port})$$

When segments from both instances arrive at the server's physical network interface, both segments have destination IP `198.60.22.13` and destination port `80`. The server's OS inspects the transport header and hashes all four fields. Because the source ports differ (`53687` vs `53686`), the segments map to two distinct socket descriptors in the server's socket table. The server processes their HTTP requests completely independently without stream corruption.

---

### 12. Slow Start

#### File and Segment Parameters
- File size of `Packages.gz`: $13,332,733\text{ bytes}$
- MSS: $1,460\text{ bytes}$
- Total segments:
  $$N_{\text{segments}} = \left\lceil \frac{13,332,733}{1,460} \right\rceil = \mathbf{9,133\text{ segments}}$$

#### RTTs to Send File in Slow Start ($W_1 = 10$, doubling each RTT)
- **Round 1:** $10\text{ segments}$ (Cumulative: 10)
- **Round 2:** $20\text{ segments}$ (Cumulative: 30)
- **Round 3:** $40\text{ segments}$ (Cumulative: 70)
- **Round 4:** $80\text{ segments}$ (Cumulative: 150)
- **Round 5:** $160\text{ segments}$ (Cumulative: 310)
- **Round 6:** $320\text{ segments}$ (Cumulative: 630)
- **Round 7:** $640\text{ segments}$ (Cumulative: 1,270)
- **Round 8:** $1,280\text{ segments}$ (Cumulative: 2,550)
- **Round 9:** $2,560\text{ segments}$ (Cumulative: 5,110)
- **Round 10:** $5,120\text{ segments}$ (Cumulative: 10,230 $\ge 9,133$)

**It takes exactly 10 RTTs to transmit the entire file.**

#### Shortest Theoretical Transfer Time ($10 \times \text{RTT}_{\text{min}}$) vs. Measured

| Vantage Point | Path | Min Connect (RTT) | Theoretical Minimum ($10 \times \text{RTT}$) | Measured Transfer Time |
| :--- | :--- | :--- | :--- | :--- |
| **Codespace** | **Utah** | **31.1 ms** | **311.0 ms (0.31 s)** | **341.0 ms (0.34 s)** |
| **Codespace** | **Japan** | **108.2 ms** | **1,082.0 ms (1.08 s)** | **4,152.5 ms (4.15 s)** |
| **Laptop** | **Utah** | **43.4 ms** | **434.0 ms (0.43 s)** | **631.7 ms (0.63 s)** |
| **Laptop** | **Japan** | **160.5 ms** | **1,605.0 ms (1.61 s)** | **10,485.9 ms (10.49 s)** |
| **Onyx** | **Utah** | **49.8 ms** | **498.0 ms (0.50 s)** | **1,155.4 ms (1.16 s)** |
| **Onyx** | **Japan** | **199.1 ms** | **1,991.0 ms (1.99 s)** | **65,634.9 ms (65.63 s)** |

#### Which Path Comes Closer to the Estimate?
**Utah comes dramatically closer to the theoretical slow start estimate.**
In Codespace, the Utah transfer took **341.0 ms**, coming within 30 ms of the theoretical minimum of **311.0 ms** (achieving **91% of theoretical ideal efficiency**).

#### What is Holding Japan Back? (Sections 3.5 & 3.7)
1. **TCP Receive Window (`rwnd`) Clamping:**
   Round 10 requires a window of $5,120 \times 1,460\text{ bytes} \approx 7.48\text{ MB}$. Client and server OS socket buffers rarely allow `rwnd` to scale beyond 1–2 MB. Once `cwnd` hits `rwnd`, TCP can no longer double; it stalls into linear throughput.
2. **High-RTT Loss Penalty (Mathis Formula):**
   Across an 8,500 km trans-Pacific route with 20+ hops, minor buffer overflows cause occasional segment drops. In TCP congestion control, loss cuts the congestion window in half ($cwnd \leftarrow cwnd / 2$). In congestion avoidance, window recovery increases by only 1 MSS per RTT. When $\text{RTT} \approx 200\text{ ms}$, window recovery takes 4× to 6× longer in wall-clock time than Utah. As stated by the throughput formula:
   $$\text{Throughput} \le \frac{1.22 \times \text{MSS}}{\text{RTT} \times \sqrt{p}}$$
   high RTT acts as a severe multiplier on throughput degradation.

---

## Chapter 4: The Network Layer

### 13. Private and Public Addresses

From our measurement headers and `checkip.amazonaws.com` outputs:

| Vantage Point | Local Address (4-tuple) | Public Address (`checkip`) | RFC 1918 Private? | Behind NAT? | Evidence |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Laptop** | `192.168.4.83` | `160.2.119.105` | **Yes** (`192.168.0.0/16`) | **Yes** | Local IP $\neq$ Public IP |
| **Onyx** | `132.178.227.11` | `132.178.227.11` | **No** (Public Class B) | **No** | Local IP $=$ Public IP |
| **Codespace** | `10.0.1.242` | `4.155.74.51` | **Yes** (`10.0.0.0/8`) | **Yes** | Local IP $\neq$ Public IP |

#### Summary of Evidence
- **Laptop:** The Wi-Fi interface IP is `192.168.4.83` (within RFC 1918 block `192.168.0.0/16`), while Amazon reports traffic arriving from Sparklight public WAN IP `160.2.119.105`. The local router translated the IP, proving NAT.
- **Codespace:** The VM container interface is `10.0.1.242` (within RFC 1918 block `10.0.0.0/8`), while Amazon observes Microsoft Azure WAN IP `4.155.74.51`. This proves an Azure NAT gateway sits between the VM and the public Internet.
- **Onyx:** The network interface `eno12399np0` is configured directly with public IP `132.178.227.11/25`, and Amazon receives packets directly from `132.178.227.11`. There is no address translation; Onyx has direct public Internet routing.

---

### 14. The NAT Table

For the **Laptop**, which is confirmed to be behind a NAT, here is the NAT translation table entry for its connection to `mirrors.xmission.com:80`, drawn according to **Kurose & Ross Section 4.3 (Figure 4.25)**:

#### NAT Translation Table Entry

| WAN Side (Internet-Facing) | LAN Side (Internal Network) |
| :--- | :--- |
| `160.2.119.105 : 53687` *(or router-mapped port)* | `192.168.4.83 : 53687` |

#### Observability Analysis
- **What we CAN observe:**
  - **LAN Side IP (`192.168.4.83`):** Directly observed via `getsockname()` on the laptop.
  - **LAN Side Port (`53687`):** Directly observed via `getsockname()` as the local socket ephemeral port.
  - **WAN Side IP (`160.2.119.105`):** Directly observed via our HTTP query to `checkip.amazonaws.com`.
- **What we CANNOT observe:**
  - **WAN Side Port:** We cannot determine the exact external port number assigned by the home router's NAT table for this specific outbound connection.
- **Why we cannot see it:**
  NAT translation occurs inside the gateway router's kernel state table after the IP packet leaves the laptop's network card. The laptop OS only knows its internal socket bindings. Plain HTTP servers do not echo back the observed TCP source port in their payload. Without an out-of-band diagnostic protocol (like STUN - RFC 5389) or external packet captures on the WAN side of the router, internal clients have zero visibility into the NAT gateway's external port allocation.

---

### 15. Subnetting

Extracting interface addresses and prefix lengths from the measurement headers:

1. **Laptop:** `192.168.4.83/22` (Netmask `0xfffffc00` = `255.255.252.0`)
2. **Onyx:** `132.178.227.11/25` (Netmask `255.255.255.128`)
3. **Codespace:** `10.0.1.242/16` (Netmask `255.255.0.0`)

#### Subnet Calculations Table

| Metric | Laptop (`en0`) | Onyx (`eno12399np0`) | Codespace (`eth0`) |
| :--- | :--- | :--- | :--- |
| **Interface CIDR** | `192.168.4.83/22` | `132.178.227.11/25` | `10.0.1.242/16` |
| **Subnet Netmask** | `255.255.252.0` | `255.255.255.128` | `255.255.0.0` |
| **Network Address** | `192.168.4.0` | `132.178.227.0` | `10.0.0.0` |
| **Broadcast Address** | `192.168.7.255` | `132.178.227.127` | `10.0.255.255` |
| **Usable Host Range** | `192.168.4.1` – `192.168.7.254` | `132.178.227.1` – `132.178.227.126` | `10.0.0.1` – `10.0.255.254` |
| **Total Usable Hosts**| $2^{10} - 2 = \mathbf{1,022}$ | $2^{7} - 2 = \mathbf{126}$ | $2^{16} - 2 = \mathbf{65,534}$ |

#### Binary Demonstration for Onyx (`132.178.227.11/25`)

```text
IP Address:        10000100 . 10110010 . 11100011 . 00001011  (132.178.227.11)
Subnet Mask (/25): 11111111 . 11111111 . 11111111 . 10000000  (255.255.255.128)
--------------------------------------------------------------------------------
Network (AND):     10000100 . 10110010 . 11100011 . 00000000  (132.178.227.0)
Host Bits (NOT):   00000000 . 00000000 . 00000000 . 01111111  (127 hosts)
Broadcast (OR):    10000100 . 10110010 . 11100011 . 01111111  (132.178.227.127)
```

---

# Task 6: Catch the Agent

### Agent Correction 1: Premature Code Generation vs. Explicit Approval Rule
- **Quote from Agent Session:**
  During Phase 1, the agent attempted to immediately invoke the `write_to_file` tool to create `pageload.c` without waiting for the user to explicitly approve the implementation plan.
- **User Intervention:**
  The user explicitly denied permission and commanded:
  > *"I want a plan to approve. Do NOT write any code until I have approved. Make a memory that I want to approve a plan for all projects before you start implementing."*
- **Resolution and Independent Check:**
  The agent recognized the violation of the pair programming workflow, stopped tool execution, presented the plan for user approval, and permanently registered the user preference into `~/.gemini/config/GEMINI.md`.

---

### Agent Correction 2: SCP vs. Git Cloning on Onyx
- **Quote from Agent Session:**
  The agent proposed:
  ```bash
  ssh onyx "mkdir -p cs425-x1" && scp pageload.c Makefile onyx:cs425-x1/
  ```
- **User Intervention:**
  The user caught that the project was already tracked in git and asked:
  > *"can't you just clone the git repo there instead? git@github.com:harrisonjansenva/cs415-x1.git"*
- **Resolution:**
  Cloning the git repository directly on Onyx avoided unsandboxed local file copying, automatically retained version control tracking (`.git`), and ensured that `git rev-parse --short HEAD` in `measure.sh` accurately reported the git commit `337016c` across all three measurement files.

---

### Independent Technical Verification: NAT Status on Onyx
- **Claim:** The agent stated during pre-measurement planning that Onyx would likely have a public IP and not be behind a NAT, contradicting the initial prediction that all three vantage points would be behind a NAT.
- **Verification Method:**
  1. Inspected `measurements-onyx.txt` line 11: `inet 132.178.227.11/25`
  2. Inspected `checkip.amazonaws.com` line 22: `132.178.227.11`
  3. Ran `whois 132.178.227.11` to confirm AS allocation:
     ```text
     NetRange:       132.178.0.0 - 132.178.255.255
     CIDR:           132.178.0.0/16
     OrgName:        Boise State University
     OrgId:          BSU-2
     ```
  Because the local interface address is identical to the public WAN address observed by AWS, and the address is an ARIN-registered public unicast IP belonging to Boise State University (not RFC 1918), this verified that Onyx is directly routable on the public Internet without NAT translation.