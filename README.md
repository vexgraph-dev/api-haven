# anti-api-haven

Welcome to **anti-api-haven**, the high-performance, zero-allocation volatile API declaration library for the Anti Engine ecosystem.

## 🚀 Overview
This library manages the structures, endpoint queries, and network schemas for the Anti Engine. It is structured as a nested Git repository so it can be dynamically modified and shared across other services and applications as an active dependency.

## ⚠️ Volatile Design Contract
*   This library is **highly volatile**. Primitives, Structs, and Off-heap memory layout definitions here can change dynamically as the engine core evolves.
*   All additions and alterations must adhere to zero-GC design principles and prioritize direct memory downcall mappings.

## 🛠️ Package Conventions
*   Package Name: `api.*`
*   All code must reside under the root directory `/src/api/`.
