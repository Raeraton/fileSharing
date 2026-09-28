
## server

### api

#### general api format: 
request
- request size >= 8
- req[:4] -> type
- req[4:8] -> req id (could be anything and echoed back in the response)
response
- no request can be longer than packet size or 4096 byte (the mtu is the normal upper bound)
- minimum packet size is 64
- response >= 4
- resp[:4] = req id (multiply response is possible for one request)



#### set package size

request: [10]
- req type = 'PKGS'
- req[8:10] -> package size (u16) (min 64)

response: [6]
- resp[4:6] = package size (if it already had a value than that)

#### get file size

request: [8]
- req type = 'GFS_'

response: [12]
- resp[4:12] = filesize

#### get packet by id

request: [12]
- req type = 'GPBI'
- req[8:12] -> packet id

response: 8 < []
- resp[4:8] = packet id
- resp[8:] = packet

#### get packet range

request: [16]
- req type = 'GRNG'
- req[8:12] -> range start (included)
- req[12:16] -> range end (exclude)

response: 
- resp[4:8] = packet id
- resp[8:] = packet

#### get packet by indecies

request: 8 + n*4
- req type = 'GBIS'
- req[8:] = u32 indecies

response:
- resp[4:8] = packet id
- resp[8:] = packet

#### finish

request:
- req type == 'FINH'