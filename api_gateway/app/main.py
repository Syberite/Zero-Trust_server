from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from app.routes import vault, auth

app = FastAPI(
    title="Zero-Trust Password Manager API",
    description="Python Frontend connected to a custom C B+ Tree Database",
    version="1.0.0"
)

# Allows our api_demo.html to talk to the API
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(vault.router)
app.include_router(auth.router)

@app.get("/")
def read_root():
    return {"message": "Welcome to the Zero-Trust API Gateway. System is Online."}
