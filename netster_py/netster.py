import argparse

import socket, time

def recvfrom(self, sz, flags=0):
    data = self._realrecvfrom(sz, flags)
    time.sleep(getattr(recvfrom, "delay", 0) / 1000)
    return data

def sendto(self, data, flags, addr=None):
    sendto.pkt = getattr(sendto, "pkt", 0) + 1
    period = getattr(sendto, "period", 0)
    seq = getattr(sendto, "seq", [])
    if (period > 0 and sendto.pkt % period == 0): return len(data)
    elif sendto.pkt in seq: return len(data)

    if addr is None: addr, flags = flags, 0
    self._realsendto(data, flags, addr)

socket.socket._realsendto = socket.socket.sendto
socket.socket._realrecvfrom = socket.socket.recvfrom
socket.socket.sendto = sendto
socket.socket.recvfrom = recvfrom

def noimp(*args, **kwargs):
    raise NotImplementedError()

try:
    from filetransfer import file_server, file_client
except ImportError:
    file_server = file_client = noimp
try:
    from stopandwait import stopandwait_server, stopandwait_client
except ImportError:
    stopandwait_server = stopandwait_client = noimp
try:
    from gobackn import gbn_server, gbn_client
except ImportError:
    gbn_server = gbn_client = noimp
try:
    from chat import chat_server, chat_client
except ImportError:
    chat_server = chat_client = noimp

def run_server(args, fp):
    fns = [lambda: file_server(args.iface, args.port, args.udp, fp),
           lambda: stopandwait_server(args.iface, args.port, fp),
           lambda: gbn_server(args.iface, args.port, fp)]
    if fp: fns[args.rudp]()
    else: chat_server(args.iface, args.port, args.udp)

def run_client(args, fp):
    fns = [lambda: file_client(args.host, args.port, args.udp, fp),
           lambda: stopandwait_client(args.host, args.port, fp),
           lambda: gbn_client(args.host, args.port, fp)]
    if fp: fns[args.rudp]()
    else: chat_client(args.host, args.port, args.udp)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", "--port", type=int, default=12345, help="listen on/connect to port <port>")
    parser.add_argument("-i", "--iface", type=str, help="listen on interface <dev>")
    parser.add_argument("-f", "--file", type=str, help="file to read/write")
    parser.add_argument("-u", "--udp", action="store_true", help="use UDP (default TCP)")
    parser.add_argument("-r", "--rudp", type=int, default=0, choices=[0,1,2], help="use RUDP (1=stopwait, 2=gobackN)")
    parser.add_argument("host", type=str, default="", nargs="?", help="when included, starts a client connecting to <host>")
    args = parser.parse_args()
    fp = None

    if args.udp and args.rudp:
        print("ERROR: Usage - '--udp' and '--rudp' flags cannot both be set greater than 1")
        parser.print_help()
        exit(-1)
    if args.rudp and not args.file:
        print("ERROR: Usage - the '--rudp' flag must also specify a '--file'")
        parser.print_help()
        exit(-1)

    if args.file:
        fp = open(args.file, ( "rb" if args.host else "wb"))

    try:
        # delay = ##
        # period = ##
        # sequence = #,#,#
        with open("socket.cfg", "r") as f:
            for l in f:
                if l.strip()[0] != "#":
                    try:
                        k,v = l.split('=')
                        if k.strip().lower() == "delay" and v:      recvfrom.delay = int(v)
                        elif k.strip().lower() == "period" and v:   sendto.period = int(v)
                        elif k.strip().lower() == "sequence" and v: sendto.seq = [int(x) for x in v.split(',')]
                    except ValueError: pass
    except OSError: pass
        
    if args.host: run_client(args, fp)
    else: run_server(args, fp)

    if args.file:
        fp.close()
