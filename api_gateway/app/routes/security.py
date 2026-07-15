from fastapi import APIRouter
from app.services.generator import generate_strong_password

router = APIRouter(prefix="/security", tags=["Security"])

@router.get("/generate")
def generate_password(length: int = 16):
    """Generates a secure random password without storing it."""
    if length <= 0 or length > 128:
        return {"error": "Length must be between 1 and 128"}
        
    new_password = generate_strong_password(length)
    return {"generated_password": new_password}
