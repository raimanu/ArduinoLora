import pandas as pd
import psycopg2
from datetime import datetime


# Database connection parameters (adjust as needed)
DB_HOST = "localhost"
DB_NAME = "your_database"
DB_USER = "your_username"
DB_PASSWORD = "your_password"
devEUI = "fictitious_device"

# Example PostgreSQL connection function
def insert_data_into_db(devEUI, timestamp, data_type, data_value):
    try:
        # Establish a connection to the database
        conn = psycopg2.connect(
            database="bddLoRaV2", 
            user="postgres", 
            password="admin", 
            host="127.0.0.1", 
            port="5432"
        )
        cur = conn.cursor()
        
        # Insert data into the table (you can modify the table and columns)
        query = """INSERT INTO "Data" ("devEUI", time, type, data) VALUES (%s, %s, %s, %s)"""

        cur.execute(query, (devEUI, timestamp, data_type, data_value))
        
        # Commit the transaction
        conn.commit()
        
        # Close the cursor and connection
        cur.close()
        conn.close()
        
    except Exception as e:
        print(f"Error inserting data into PostgreSQL: {e}")


# Processing data from the DataFrame Q_01_latest-2023-2024_RR-T-Vent.csv
data = pd.read_csv('D:/1.Master/semestre3/Projet_tut/ArduinoLora/StructureData/Q_01_latest-2023-2024_RR-T-Vent.csv', sep=';')

for i in range(0, 100):
    
    # Convert the hour from decimal to HHMM format by taking the first 2 digits as hours and the last 2 as minutes
    time_str = int(data.iloc[i, 10])
    time_str = str(time_str).zfill(4)
    
    # Convert the date and time to a timestamp format
    timestamp = datetime.strptime(str(data.iloc[i, 5]) + time_str, '%Y%m%d%H%M').strftime('%Y-%m-%d %H:%M:%S')
    print(timestamp)

    # Insert tempAir (assuming TN as air temperature), collumn 9
    insert_data_into_db(devEUI, timestamp, 'tempAir', int(data.iloc[i, 8]))

    # Insert altitude from the collumn ALTI
    insert_data_into_db(devEUI, timestamp, 'altitude', int(data.iloc[i, 4]))