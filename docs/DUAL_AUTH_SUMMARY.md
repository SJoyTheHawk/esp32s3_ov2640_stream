# Dual-Tier Authentication System Summary

## Overview

The ESP32-S3 Camera system now supports **two levels of user access**:

| Role | Access Level | Default Credentials |
|------|-------------|---------------------|
| **Admin** | Full system access | Username: `admin`, Password: `admin` |
| **User** | Camera-only access | Username: `user`, Password: `user` |

---

## Access Control Matrix

| Feature / Endpoint | Admin | User | Public |
|-------------------|-------|------|--------|
| **Stream Viewing** (`/stream`) | ✅ | ✅ | ❌ |
| **Snapshot Capture** (`/capture`) | ✅ | ✅ | ❌ |
| **Camera Settings** (`/api/camera/config`) | ✅ | ✅ | ❌ |
| **WiFi Settings** (`/api/settings`) | ✅ | ❌ | ❌ |
| **Network Settings** (`/api/settings`) | ✅ | ❌ | ❌ |
| **Change Own Password** (`/api/change-password`) | ✅ | ✅ | ❌ |
| **Change Any Password** (`/api/change-password`) | ✅ | ❌ | ❌ |
| **System Restart** (`/api/restart`) | ✅ | ❌ | ❌ |
| **Factory Reset** (`/api/factory-reset`) | ✅ | ❌ | ❌ |

---

## Implementation Status

### ✅ Already Implemented

**Phase 1 - Storage (camera_settings.h/.cpp):**
- Separate credential storage for admin and user
- NVS keys: `adminUser`, `adminPass`, `userUser`, `userPass`
- Default values defined in `DefaultValues` struct
- Read/write methods for both credential sets

**Phase 2 - Web Server (web_server.h/.cpp):**
- Dual session token management (`adminAuthToken_`, `userAuthToken_`)
- Independent session timeouts for each role
- `getAuthLevel()` - returns `ADMIN`, `USER`, or `NONE`
- `isAuthenticated()` - checks for any valid login
- `isAdminAuthenticated()` - checks for admin-only access
- `handleLogin()` - validates credentials and sets role-based tokens

### 🔨 To Be Completed in Phase 8

**WiFi Provisioning Integration:**
- Update setup page HTML to collect both admin and user credentials
- Modify `/api/configure` endpoint to save all four credentials during first-time setup
- Update JavaScript to submit all credential fields

**API Endpoint Access Control:**
- Add `isAdminAuthenticated()` checks to admin-only endpoints
- Add `isAuthenticated()` checks to camera endpoints
- Update `handleChangePassword()` to enforce role-based restrictions

**UI Role-Based Visibility:**
- Hide WiFi/network settings sections for user role
- Hide system management buttons (restart, reset) for user role
- Show role indicator in UI ("Logged in as: Admin" or "User")

---

## Code Locations

All dual authentication code is documented in:

1. **IMPLEMENTATION_GUIDE.md**
   - Phase 1: Storage structure (lines 31-180)
   - Phase 2: Authentication handlers (lines 602-850)
   - Phase 8: Integration details (section 8.0)

2. **IMPLEMENTATION_PLAN.md**
   - Phase 1: Settings structure (lines 24-50)
   - Phase 2: Authentication system (lines 54-106)
   - Phase 8: Integration tasks (lines 283-300)

---

## Testing Checklist

- [ ] Admin login with correct credentials
- [ ] User login with correct credentials
- [ ] Invalid credentials rejected
- [ ] Admin can access all endpoints
- [ ] User can access camera endpoints
- [ ] User CANNOT access settings endpoints (403 Forbidden)
- [ ] User can change own password
- [ ] User CANNOT change admin password
- [ ] Admin can change any password
- [ ] Sessions timeout after 30 minutes
- [ ] Multiple simultaneous logins (admin + user)
- [ ] First-time setup collects both credential sets
- [ ] Factory reset clears both credential sets

---

## Security Considerations

1. **Session Management:**
   - 30-minute timeout for both roles
   - HttpOnly cookies prevent XSS access
   - Separate tokens prevent privilege escalation

2. **Password Storage:**
   - Stored in NVS (ESP32 flash)
   - Consider adding password hashing in production (bcrypt/SHA-256)

3. **Default Credentials:**
   - MUST be changed during first-time setup
   - Provisioning page should enforce strong passwords

4. **Network Security:**
   - Use HTTPS if possible (requires certificate)
   - Consider WPA2/WPA3 Enterprise for production WiFi

---

## Future Enhancements

- [ ] Password strength validation (min 8 chars, special chars)
- [ ] Password hashing (bcrypt) instead of plaintext storage
- [ ] Session token encryption
- [ ] Rate limiting on login attempts
- [ ] Audit log of admin actions
- [ ] User management UI (add/remove users)
- [ ] Guest access with view-only stream
- [ ] API key authentication for Python clients

---

**Last Updated:** 2026-08-27
