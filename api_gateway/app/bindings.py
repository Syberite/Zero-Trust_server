from app.db_client import ZeroTrustClient

# 1. We instantiate the connection to your C Database exactly ONCE when the app starts.
# This prevents opening hundreds of sockets if multiple users connect to your web app.
db_connection = ZeroTrustClient(host='127.0.0.1', port=8080)

def get_db():
    """
    FastAPI Dependency.
    Any web route that needs to talk to the database will call this function.
    """
    return db_connection
