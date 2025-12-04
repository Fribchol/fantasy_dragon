## Spielfigur
- Steuerung über **WASD**  
  - **A/D**: links/rechts  
  - **W/S**: Bewegung in die Tiefe innerhalb eines begrenzten Bodenbereichs  
- Kann **springen** (Salto); während des Sprungs immun gegen Schaden und kann über Gegner hinweg  
- **HP** und **Mana** regenerieren sich langsam automatisch  
- Kann **schlagen** und **zaubern**  
- Hebt **Items** von besiegten Gegnern automatisch auf

## Gegner
- Erscheinen abschnittsweise in sogenannten **Arenen**  
- Jeder Gegnertyp besitzt **nur ein Angriffsmuster**  
- Bewegen sich automatisch auf die Spielfigur zu  
- Versionen **mit** und **ohne Waffe**  
- Besitzen **X Lebenspunkte** bzw. halten X Treffer aus

## Items
- **Schwert**:
  - Hebt der Spieler automatisch auf
  - Besitzt eine **begrenzte Haltbarkeit** von **X Schlägen**, unabhängig davon, ob ein Treffer landet

## View
- **Seitliche Ansicht**, aber mit **Tiefe** (Bewegung innerhalb eines oberen und unteren Bodenlimits)  
- Kamera ist **spielerzentriert**, bis die Spielfigur eine **Arena** betritt  
- Während einer Arena bleibt die Kamera **fixiert**, bis die Arena abgeschlossen und verlassen wird

## Level
- **Leitern**:
  - Spieler benutzt sie automatisch, indem er dagegen läuft  
  - Starten immer an der **oberen Bodenbegrenzung**  
  - **W** gedrückt halten → hochklettern  
  - **S** gedrückt halten → herunterklettern
