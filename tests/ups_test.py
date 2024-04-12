from pysnmp.hlapi import *

# Dirección IP de la tarjeta PIS301
target_ip = '192.168.1.100'
# Comunidad SNMP (por defecto suele ser 'public')
community = 'public'

# OID para obtener el estado de la batería de la UPS
battery_status_oid = '1.3.6.1.4.1.318.1.1.1.2.2.1.0'

# Función para realizar la consulta SNMP
def snmp_get(oid):
    errorIndication, errorStatus, errorIndex, varBinds = next(
        getCmd(SnmpEngine(),
               CommunityData(community),
               UdpTransportTarget((target_ip, 161)),
               ContextData(),
               ObjectType(ObjectIdentity(oid)))
    )

    if errorIndication:
        print(errorIndication)
        return None
    elif errorStatus:
        print('%s at %s' % (errorStatus.prettyPrint(),
                            errorIndex and varBinds[int(errorIndex) - 1][0] or '?'))
        return None
    else:
        for varBind in varBinds:
            return varBind[1]

# Obtener el estado de la batería
battery_status = snmp_get(battery_status_oid)

if battery_status is not None:
    print('Estado de la batería de la UPS:', battery_status)
else:
    print('No se pudo obtener el estado de la batería de la UPS')
