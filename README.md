# powsys
Jerome Powell's system for automated bug bounty recon, scanning, and reporting (to print tendies)

## Approach
- Write as a multithreaded, multi-component pipeline with various modules
- Write modules as portable components that provide a focused subset of their bash tool counterparts' functionality
- Write components in one library and the C++ binary as an example program using this library
- Pipeline steps, all of which may be done in parallel, to produce a final set of documents to investigate:
    1. Subdomain enumeration (amass, subfinder)
    2. Concurrent steps to process subdomain: (1) subdomain takeover (subjack), (2) CORS (CORScanner), (3) screenshots (aquatone), (4) URLs recursively (aquatone)
    3. Concurrent resolving of subdomains into IP addresesses (massdns)
    4. Scanning of IP addresses for open ports (masscan)
    5. Nmap scanning for version and scripts of open ports (nmap)
