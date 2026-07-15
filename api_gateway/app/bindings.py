from app.db_client import ZeroTrustClient

# Instantiate the connection exactly ONCE to prevent socket exhaustion
db_connection = ZeroTrustClient(host='127.0.0.1', port=8080)

def get_db():
    """
    FastAPI Dependency.
    Routes will call this to talk to the C Database.
    """
    return db_connection
