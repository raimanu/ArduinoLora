import paho.mqtt.client as mqtt #import the client1
import psycopg2
from datetime import datetime
import time
import json
import requests
import ssl

#------------------------------------------------------------------------------------------------
def on_log(client, userdata, level, buf) :
            k=0
#------------------------------------------------------------------------------------------------
def on_connect(client, userdata, flags, rc) :
        if rc==0:
                print("connected ok")
        else:
                print("not connected", rc)
#------------------------------------------------------------------------------------------------
def on_disconnect(client, userdata, flags, rc=0) :
        print("disconnect result code "+str(rc))
#------------------------------------------------------------------------------------------------
def on_message(client,userdata,msg) :
        global m_decode
        topic=msg.topic
        trameLora = msg.payload.decode("utf-8","ignore")
        trameLora = json.loads(trameLora)
        addData(trameLora)
#------------------------------------------------------------------------------------------------
def addData(trameLora) :
        conn = psycopg2.connect(database="guillaume_lora", user='guillaume', password='guillaume', host='127.0.0.1', port= '5432')
        conn.autocommit = True
        cursor = conn.cursor()
        devEUI =  str(trameLora["devEUI"])
        cursor.execute('''SELECT EXISTS (SELECT "devEUI" FROM "Sensor" WHERE "devEUI" = %s)''' , (devEUI,))
        if not (cursor.fetchone()[0]) :
                values = (trameLora["devEUI"],trameLora["deviceName"])
                cursor.execute('''INSERT INTO "Sensor"("devEUI","deviceName") VALUES (%s,%s)''', values)
                print("New Sensor added with Success")
        time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        dictData = trameLora["object"]
        for type, data in dictData.items():
                values = (devEUI,time, type, data)
                cursor.execute('''INSERT INTO "Data"("devEUI", "time", "type", "data") VALUES (%s, %s, %s, %s)''', values)
        print("All data added with Sucess")
        conn.commit()
        conn.close()
#------------------------------------------------------------------------------------------------

broker_address="127.0.0.1:1883"

client = mqtt.Client("loraWAN") #create new instance

client.on_connect=on_connect
client.on_disconnect=on_disconnect
client.on_log=on_log
client.on_message=on_message

print ("connect to broker", broker_address)
client.connect("127.0.0.1", 1883, 60)
client.subscribe("application/1/#")
client.loop_forever()

broker_address="127.0.0.1:1883"

client = mqtt.Client("loraWAN") #create new instance

client.on_connect=on_connect
client.on_disconnect=on_disconnect
client.on_log=on_log
client.on_message=on_message

print ("connect to broker", broker_address)
client.connect("127.0.0.1",1883,60)
client.subscribe("application/1/#")
client.loop_forever()