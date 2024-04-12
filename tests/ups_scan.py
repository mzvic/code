import nmap
from pysnmp.hlapi import *

# Función para realizar la consulta SNMP
def snmp_get(oid, ip, community='public', port=161, timeout=1, retries=0):
    errorIndication, errorStatus, errorIndex, varBinds = next(
        getCmd(SnmpEngine(),
               CommunityData(community),
               UdpTransportTarget((ip, port), timeout=timeout, retries=retries),
               ContextData(),
               ObjectType(ObjectIdentity(oid)))
    )

    if errorIndication:
        return None
    elif errorStatus:
        return None
    else:
        for varBind in varBinds:
            return varBind[1]

# Función para escanear la red en busca de dispositivos UPS
def scan_for_ups():
    nm = nmap.PortScanner()
    nm.scan(hosts='152.74.216.0/24', arguments='-p 161 --open')

    for host in nm.all_hosts():
        try:
            sys_descr = snmp_get('1.3.6.1.2.1.1.1.0', host)
            if sys_descr and 'UPS' in str(sys_descr):
                print(f'UPS encontrada en {host}')
        except Exception as e:
            pass

# Escanear la red en busca de UPS
scan_for_ups()

