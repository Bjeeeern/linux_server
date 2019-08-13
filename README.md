# linux_server
Denna HTTP server kördes på en RASPBERRY PI 3 MODEL B med adressen http://seyama.se men är för tillfället nere.

Det var en hemserver utan WANip så jag använde ngrok(https://ngrok.com/) för att hosta. <br/>
Freedns(http://freedns.afraid.org) var min DNS server och domännamnet var köppt av loopia(https://www.loopia.se/)

## om web sidan ##

Användes som min portfolio

## Om servern ##

Servern består av en core loop som laddar flera DLL moduler och laddar om dem om den hittar en nyare verision efter omkompilering.

När en TCP connection kommer in till servern så beroende på medelandet så forkar serven en motsvarande modul med 1mb tillgodo. När uppkopplingen avbryts eller avslutas så avslutas processen och det tilldelade minnet returneras.

T.ex. github_webhook.cpp beskriver en HTTP modul som svarar på medelande med formen "POST /github-push-event" som skickas när detta repository har uppdaterats. Då laddar den ner den senaste verisionen och omkompilerar och laddar om hela servern lokalt.
