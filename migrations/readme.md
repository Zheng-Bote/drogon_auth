Zusätzliche Hinweise / Empfehlungen (konkret)
Argon2id: Verwende in der Anwendung argon2id (nicht argon2i oder argon2d). Speichere den vollen Encoded‑String ($argon2id$v=19$m=...,t=...,p=...$salt$hash) in password_hash. Empfohlene Startwerte (konfigurierbar via .env):

ARGON2_MEMORY=65536 (KB)
ARGON2_ITERATIONS=3
ARGON2_PARALLELISM=1

Passe je nach Hardware an; dokumentiere in README wie man testet (time/memory tradeoff).

TOTP Secret Schutz: totp_secrets.secret ist sensibel. Ziehe in Betracht, Secrets in der App zu verschlüsseln (z. B. mit einem KMS oder einem App‑Master‑Key, der nicht in der DB liegt). Markiere das Feld als sensibel und beschränke DB‑User‑Rechte.

Kommunikationsdaten: user_communications.metadata erlaubt provider‑spezifische Felder (z. B. {"chat_id":"12345","device":"iPhone"}). is_active und verified erlauben, Kanäle zu deaktivieren oder zu verifizieren.

Sessions: session_token sollte mindestens 32 Bytes kryptographisch sicher sein, in Base64/hex gespeichert. expires_at muss gesetzt werden; last_accessed_at für Sliding‑TTL falls gewünscht.

Brute‑Force / Rate‑Limiting: Nutze login_attempts oder Redis für schnelle Sperrung nach N Fehlschlägen. Logge fehlgeschlagene Versuche in audit_logs/login_attempts.

Soft‑Delete: is_active ermöglicht Sperrung statt Löschung. Wenn du Löschanforderungen (GDPR) hast, implementiere deleted_at und Lösch‑Workflows.

Indizes: Bereits gesetzt für häufige Abfragen (loginname, email, user_id, expires_at).

UUID‑Generierung: In SQLite‑Fallback generiert die App UUIDs; in Postgres nutzen wir gen_random_uuid() (pgcrypto).

Migrationen: Halte Migrationen in sequenziellen Dateien (001*..., 002*...) und führe sie mit einem Migrations‑Tool (z. B. sqitch, migrate, oder ein eigenes make migrate Script).
