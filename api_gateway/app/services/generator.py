import secrets
import string

def generate_strong_password(length: int = 16) -> str:
    """
    Generates a cryptographically secure random password using Python's 'secrets' library.
    This replaces the old C rand() function, providing much better security.
    """
    # Define the pool of characters to choose from
    alphabet = string.ascii_letters + string.digits + "!@#$%^&*()_-+=<>?"
    
    # Securely pick random characters
    password = ''.join(secrets.choice(alphabet) for _ in range(length))
    
    return password
