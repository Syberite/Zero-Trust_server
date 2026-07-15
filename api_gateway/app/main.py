from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from app.routes import vault, auth

# Initialize the FastAPI application
app = FastAPI(
    title="Zero-Trust Password Manager API",
    description="A Python Frontend connected to a custom C B+ Tree Database via TCP Sockets",
    version="1.0.0"
)

# This allows external HTML files (like our demo) to talk to the API
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Plug in the routes we just created
app.include_router(vault.router)
app.include_router(auth.router)

@app.get("/")
def read_root():
    return {"message": "Welcome to the Zero-Trust API Gateway. System is Online."}
