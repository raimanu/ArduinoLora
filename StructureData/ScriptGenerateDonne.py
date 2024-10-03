import time
from datetime import datetime
import psycopg2

# Fonction pour générer des données de température
def generate_temperature():
    current_hour = datetime.now().hour
    if 8 <= current_hour < 20:
        return 25.0
    else:
        return 18.0

# Fonction pour insérer les données dans la base de données
def insert_temperature_data():
    try:
        # Connexion à la base de données
        conn = psycopg2.connect(
            database="bddLoRaV2", 
            user="postgres", 
            password="admin", 
            host="127.0.0.1", 
            port="5432"
        )
        cursor = conn.cursor()
        
        # Générer les données de température
        temperature = generate_temperature()
        time_now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        devEUI = "fictitious_device"
        type = "temperature"
        
        # Insérer les données dans la base de données
        values = (devEUI, time_now, type, temperature)
        cursor.execute('''INSERT INTO "Data"("devEUI", "time", "type", "data") VALUES (%s, %s, %s, %s)''', values)
        
        # Valider les changements et fermer la connexion
        conn.commit()
        cursor.close()
        conn.close()
        print(f"Data inserted at {time_now}: {temperature}°C")
    except Exception as e:
        print(f"Error: {e}")

# Boucle principale pour insérer les données toutes les 15 ou 30 minutes
interval_minutes = 15  # ou 30 pour 30 minutes
while True:
    insert_temperature_data()
    time.sleep(interval_minutes * 60)