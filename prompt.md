Du bist ein hochqualifizierter C++-Entwicklungsassistent. Erzeuge ein vollständiges, sofort kompilierbares **Drogon‑basierendes Authentifizierungs‑Microservice** Projekt in **C++23**, geeignet für Linux. Das Ergebnis muss alle Dateien enthalten, die zum Bauen, Testen und Deployen nötig sind: Quellcode, Header, CMakeLists, Conan v2 Konfiguration, SQL‑Migrations/Schema‑Dateien, Beispiel‑dotenv, README mit Setup/Build/Run‑Anleitung, sowie Hinweise zur Erweiterung (z. B. RBAC). Logging bleibt vorerst außen vor und muss nicht implementiert.

**Wichtige Rahmenbedingungen und Anforderungen**
- **Sprache / Tools:** C++23, Conan v2, CMake >= 3.31, Linux.
- **Webframework:** Drogon (asynchron).
- **Konfiguration:** Optionales `.env` (Pfad optional über Kommandozeile/ENV). Zugriff auf Variablen ausschließlich über `dotenv::get()` oder `std::getenv()`. (Implementiere eine kleine Config‑Wrapper‑Klasse, die zuerst dotenv::get() verwendet und fallback auf getenv()).
- **Session‑Speicherung:** Server‑seitig in Datenbank. Standard: **PostgreSQL**. Optionaler Fallback: **SQLite3** (konfigurierbar via `DB_TYPE` oder `DB_DSN`).
- **Passwort‑Hashing:** Argon2 (libargon2 oder geeignete C++ Wrapper). Verwende sichere Argon2‑Parameter (memory, iterations, parallelism) als konfigurierbare Defaults in `.env`.
- **2FA:** TOTP (RFC 6238 kompatibel). Generierung/Verifikation, QR‑Code‑Daten (otpauth://) bereitstellen; QR‑Bild selbst kann als Data‑URI oder als Text‑URI ausgegeben werden.
- **Token:** JWT nur optional für API‑Zugriff; primär server‑seitige Sessions. Wenn JWT verwendet wird, sichere Signatur mit `JWT_SECRET` aus `.env`.
- **Security:** CSRF‑Schutz für stateful Endpunkte, sichere Cookie‑Flags (`HttpOnly`, `Secure`, `SameSite`), Rate‑Limiting Hinweise (implementiere einfache in‑memory limiter als Beispiel).
- **DB‑Schema:** SQL‑Migrations/Schema‑Dateien für Postgres (und SQLite‑kompatible Varianten). Mindestens Tabellen: `users`, `sessions`, `roles`, `user_roles`, `totp_secrets`, `audit_logs` (optional).
- **API Endpoints (mindestens):**
  - `POST /api/v1/register` — Registrierung (E‑Mail, Passwort, optional TOTP aktivieren).
  - `POST /api/v1/login` — Login (E‑Mail + Passwort) → erstellt Session (DB) und setzt Session‑Cookie.
  - `POST /api/v1/logout` — Session invalidieren.
  - `POST /api/v1/token/refresh` — falls JWT/Refresh verwendet.
  - `POST /api/v1/totp/setup` — erzeugt TOTP Secret + otpauth URI (geschützt, nur für eingeloggte User).
  - `POST /api/v1/totp/verify` — verifiziert TOTP Code.
  - `GET /api/v1/me` — gibt User‑Profil zurück (auth required).
  - Admin/Dev: `POST /api/v1/admin/create-role`, `POST /api/v1/admin/assign-role` (RBAC‑Erweiterung vorbereitet).
- **Code‑Qualität:** Vollständige, gut strukturierte C++23‑Quellcode‑Dateien mit **inline Doxygen** Dokumentation für alle öffentlichen Klassen/Funktionen. Saubere Trennung: `controllers`, `services`, `repositories`, `models`, `utils`.
- **Tests:** Minimaler Unit‑Test‑Satz (z. B. Catch2) für Passwort‑Hashing, TOTP‑Verifikation und Session‑Store.
- **Build / CI:** Beispiel `conanfile.py` (Conan v2) und `CMakeLists.txt`. README beschreibt `conan install`, `cmake`, `cmake --build` Schritte. Hinweise für Dockerfile optional.
- **Erweiterbarkeit:** Code so schreiben, dass RBAC, OAuth2 Provider, externe Session Stores (Redis) leicht ergänzt werden können. Markiere Stellen mit TODO/EXTEND comments.
- **Fehlendes ergänzen:** Falls ich etwas vergessen habe, ergänze sinnvolle Defaults, Sicherheits‑Härtungen und sinnvolle Dev‑Erleichterungen (z. B. `make migrate`, `make test`).

**Ausgabeformat (sehr wichtig)**
1. Gib zuerst eine kurze Projektübersicht (2–4 Sätze).
2. Danach liste die komplette Projektstruktur als Baum (z. B. `project/`, `src/`, `include/`, `migrations/`, `tests/`, `CMakeLists.txt`, `conanfile.py`, `README.md`, `.env.example`).
3. Für jede Datei im Projektbaum: gib den vollständigen Inhalt der Datei in einem eigenen Codeblock an. Achte darauf, dass:
   - C++ Dateien vollständige, kompilierbare Implementierungen enthalten.
   - SQL Dateien vollständige CREATE TABLE / INDEX / initial seed statements enthalten.
   - `conanfile.py` für Conan v2 korrekt die Abhängigkeiten auflistet (drogon, libpqxx/libpq, sqlite3, libargon2, jwt-cpp, catch2, dotenv-cpp, otplib oder passende TOTP‑Lib).
   - `CMakeLists.txt` zeigt, wie Conan integriert wird und wie Targets erstellt werden.
   - `README.md` enthält klare Setup/Build/Run/Migration/Test Anweisungen.
4. Markiere in README und im Code die Stellen, an denen Secrets/Keys gesetzt werden müssen (z. B. `JWT_SECRET`, `DB_PASSWORD`) und gib sichere Empfehlungen (z. B. `openssl rand -hex 32`).
5. Füge am Ende eine kurze **Security‑Checklist** (Bullet‑Points) hinzu: Argon2 Parameter, HTTPS, Cookie Flags, DB‑User‑Rechte, Rate‑Limiting, Brute‑Force Schutz, Audit Logging, Secret Rotation.

**Technische Details / Libraries (Erwartung)**
- Drogon für HTTP/Controller.
- libpqxx oder libpq für Postgres; sqlite3 für optionalen lokalen Betrieb.
- libargon2 (oder libsodium falls nötig) für Argon2 Hashing.
- Eine TOTP‑Bibliothek (oder implementiere RFC6238 minimal) für TOTP.
- dotenv‑C++ (dotenv::get()) oder eine kleine Implementierung, die `.env` parst.
- jwt‑cpp (optional) für JWT‑Handling.
- Catch2 für Tests.
- Conan v2 package names sollten verwendet werden (wenn Paketname unsicher, wähle plausibel und dokumentiere).

**Konfigurationsbeispiel (.env.example)**
Füge genau dieses Beispiel als `.env.example` ein:
Database Configuration
DB_TYPE=postgres
DB_HOST=psql_server
DB_PORT=5432
DB_NAME=gallery
DB_USER=gallery_user
DB_PASSWORD=supersecret

Security
JWT_SECRET=change_me_to_a_long_random_string
TOTP_ISSUER=PhotoGallery
ARGON2_MEMORY=65536
ARGON2_ITERATIONS=3
ARGON2_PARALLELISM=1

SERVER_PORT=8848
SESSION_TTL_SECONDS=3600

Code

**Zusätzliche Anforderungen**
- Implementiere Config‑Option `DOTENV_PATH` (optional) um Pfad zur dotenv Datei zu überschreiben.
- Session IDs sollten kryptographisch sicher generiert werden (z. B. `std::random_device` + `std::uniform_int_distribution` oder `getrandom`), gespeichert als UUID/v4 oder Base64.
- SQL‑Migrations sollten idempotent sein (IF NOT EXISTS).
- Schreibe Doxygen‑kommentare für alle Controller‑Methoden, Services und Repositories.
- Implementiere einfache Input‑Validation (E‑Mail Format, Passwort Mindestlänge).
- Implementiere ein Beispiel für sichere Argon2 Parameter Auswahl und wie man sie in `.env` ändert.

**Wenn du die Aufgabe ausführst, liefere alles in einer einzigen, zusammenhängenden Antwort, ohne externe Links.**
