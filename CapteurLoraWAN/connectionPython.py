import psycopg2
from datetime import datetime

#establishing the connection
conn = psycopg2.connect(
   database="bddLoRaV2", user='postgres', password='admin', host='127.0.0.1', port= '5432'
)
#Setting auto commit false
conn.autocommit = True

#Creating a cursor object using the cursor() method
cursor = conn.cursor()

# Preparing SQL queries to INSERT a record into the database.
id_sensor = int(input("id_sensor ? : \n"))
time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
type = str(input("type ? : \n"))
data  = float(input("data ? : \n"))

values = (id_sensor,time, type, data)

cursor.execute('''INSERT INTO "Data"("id_sensor", "time", "type", "data") VALUES (%s, %s, %s, %s)''', values)


# Commit your changes in the database
conn.commit()
print("Records inserted...")

# Closing the connection
conn.close()