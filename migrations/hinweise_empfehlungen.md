# Zusätzliche Hinweise & Empfehlungen

**Notizen**

## Argon2id

Verwende in der Anwendung **argon2id** (nicht `argon2i` oder `argon2d`).  
Speichere den vollständigen Encoded‑String in `password_hash`:

```
$argon2id$v=19$m=...,t=...,p=...$salt$hash
```

### Empfohlene Startwerte (konfigurierbar via `.env`)

| Variable             | Wert  | Einheit |
| -------------------- | ----- | ------- |
| `ARGON2_MEMORY`      | 65536 | KB      |
| `ARGON2_ITERATIONS`  | 3     | –       |
| `ARGON2_PARALLELISM` | 1     | –       |

> Passe die Werte je nach Hardware an. Dokumentiere im `README`, wie man den Time/Memory-Tradeoff testet.

---

## TOTP Secret Schutz

`totp_secrets.secret` ist ein **sensibles Feld**. Empfehlungen:

- Secrets in der App verschlüsseln (z. B. mit einem KMS oder einem App‑Master‑Key, der **nicht** in der DB liegt).
- Das Feld als sensibel markieren.
- DB‑User‑Rechte auf dieses Feld beschränken.

---

## Kommunikationsdaten

`user_communications.metadata` erlaubt provider‑spezifische Felder, z. B.:

```json
{
  "chat_id": "12345",
  "device": "iPhone"
}
```

- `is_active` – Kanal deaktivieren
- `verified` – Kanal verifizieren

---

## Sessions

- `session_token` muss mindestens **32 Bytes** kryptographisch sicher sein (gespeichert als Base64 oder Hex).
- `expires_at` muss immer gesetzt werden.
- `last_accessed_at` für **Sliding‑TTL** verwenden, falls gewünscht.

---

## Brute‑Force / Rate‑Limiting

- `login_attempts` oder **Redis** für schnelle Sperrung nach _N_ Fehlschlägen nutzen.
- Fehlgeschlagene Versuche in `audit_logs` / `login_attempts` loggen.

---

## Soft‑Delete

- `is_active` ermöglicht **Sperrung statt Löschung**.
- Bei GDPR‑Löschanforderungen: `deleted_at` implementieren und entsprechende Lösch‑Workflows aufsetzen.

---

## Indizes

Bereits gesetzt für häufige Abfragen:

- `loginname`
- `email`
- `user_id`
- `expires_at`

---

## UUID‑Generierung

| Datenbank  | Methode                                          |
| ---------- | ------------------------------------------------ |
| SQLite     | UUIDs werden durch die **Applikation** generiert |
| PostgreSQL | `gen_random_uuid()` via `pgcrypto`               |

---

## Migrationen

- Migrationen in **sequenziellen Dateien** halten: `001_...`, `002_...`, usw.
- Ein Migrations‑Tool verwenden, z. B.:
  - [`sqitch`](https://sqitch.org/)
  - [`migrate`](https://github.com/golang-migrate/migrate)
  - Eigenes `make migrate` Script
