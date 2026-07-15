from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel
from app.bindings import get_db

# Create a router for all /vault endpoints
router = APIRouter(prefix="/vault", tags=["Vault"])

# Data model for what the user sends in a POST request
class PasswordData(BaseModel):
    service: str
    password: str

@router.post("/add")
def add_password(data: PasswordData, db = Depends(get_db)):
    """Encrypts and saves a password to the C Database Engine."""
    response = db.add_password(data.service, data.password)
    return {"status": "success", "message": response}

@router.get("/get/{service}")
def get_password(service: str, db = Depends(get_db)):
    """Fetches and decrypts a password from the C Database Engine."""
    response = db.get_password(service)
    
    if response.startswith("ERROR"):
        raise HTTPException(status_code=404, detail="Service not found or deleted.")
        
    return {"service": service, "password": response}

@router.delete("/delete/{service}")
def delete_password(service: str, db = Depends(get_db)):
    """Soft deletes a password from the C Database Engine."""
    response = db.delete_password(service)
    return {"status": "success", "message": response}
