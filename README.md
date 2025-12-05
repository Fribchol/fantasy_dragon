## Spielfigur
- Steuerung über **WASD**  
  - **A/D**: links/rechts  
  - **W/S**: Bewegung in die Tiefe innerhalb eines begrenzten Bodenbereichs  
- Kann **springen** (Salto); während des Sprungs immun gegen Schaden und kann über Gegner hinweg  
- **HP** und **Mana** regenerieren sich langsam automatisch  
- Kann **schlagen** und **zaubern**  
- Hebt **Items** von besiegten Gegnern automatisch auf
- Hitbox (Kleiner als der Charakter)
- Ein Hit verursacht Schaden und Rückstoß

## Gegner
- Erscheinen abschnittsweise in sogenannten **Arenen**  
- Jeder Gegnertyp besitzt **nur ein Angriffsmuster**  
- Versionen **mit** und **ohne Waffe**  
- Besitzen **X Lebenspunkte** bzw. halten X Treffer aus
- Ein Gegnertyp ist immer gleich schwierig zu besiegen = Schwierigkeit wird durch verschiedene Gegner gesteuert (DMG-Balancing)
- Ein besiegter Gegner gibt Highscorepunkte (ggf. / Nice-to-Have)

## Items
- **Schwert**:
  - Wird von Gegnern gedroppt
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
  - Am Ende eines Levels wartet ein Boss. Ist dieser besiegt, dann endet das Level. 

## Spielablauf
- Spieler läuft durchs Level
- Spieler betritt eine Arena des Levels
- Gegner bewegen sich automatisch auf die Spielfigur zu
- Spieler muss alle Gegner besiegen, um die Arena wieder verlassen zu dürfen
- Am Ende eines Levels wartet ein Boss. Ist dieser besiegt, dann endet das Level. 

## Startbildschirm
- Starten
   - InGame Pause
   - Magie-Taste
   - Spiel beenden
- Settings
  - Audio An/aus
  - Fenstergröße

## Audio
- Kampfgeräusche/Hitgeräusche
- Hintergrundmusik


## Aufgaben
  - Patrick Level Editor
  - Nadine Steuerung und Hitboxen
    - + Gegner und Spielfigur
  - Marcus Magiesystem

