# irc
IRC server / Client demo project



IRC Server & Client C:llä

- serverin vaatimukset:
	- komentoriviparametreja:
		-p, --port [port] 										portti (oletus 6667)
		-C, --connections 										maksimi yhteyksien määrä (oletus 8)
		-f, --file <file> 										tiedoston polku. Tiedosto tulostetaan jokaiselle clientille yhdistettäessä.

	- toiminnot ja ominaisuudet:
		- ottaa vastaan clientin lähettämiä irc-komentoja, joita ovat:
			komento 	alias
			/join 		/j
			/part 		/p
			/nick
			/msg
			/action 	/me
			/whois 		/w
			/topic 		/t

		- pitää kirjaa käyttäjistä, niiden tiedoista ja kanavista
			- listaa tarvittavat tietokentät suunnitelmaksi paperille
				Vinkkejä: 
					- mitkä kokonaisuudet kannattaisi ryhmitellä structeiksi?
					- mitkä muuttujat kannattaa tallentaa pointtereina? entä pointterien pointtereina?

		- virhetilanteissa antaa virheilmoituksen mutta ei pasko muodostettuja yhteyksiä tai kaadu tms
			- miten testaat, että server ei voi kaatua?


- clientin vaatimukset:
	- komentoriviparametreja:
		- (pakollinen)
		-p, --port 												portti (oletus 6667)
		-n, --nick												nimimerkki (oletus: nykyinen tietokoneen käyttäjänimi)
		--channel="<#channel1>[ #channel2[ #channel3[...]]]"	kanava[t], joille liitytään

	- toiminnot ja ominaisuudet:		
		- lähettää serverille komentoja ja vastaanottaa sekä käsittelee vastaukset
			- komentoja (joita ei mainittu server-puolessa), ovat:
				/set 											aseta yksittäinen asetus
				/save											tallenna asetukset (tämän voi tehdä viimeisenä)
				/alias 											tallenna alias. tätä pitää voida käyttää täysin vapaasti paitsi komennoille, 
																niin myös vaikkapa lyhentämään viestiä;
																esim: /alias rib "ribale räiskis" -komennon jälkeen jos lähetät viestin:
																$> ei kyllä yhtään jaksaisi syödä ribsejä
																niin lopputuloksena lähetätkin:
																$> ei kyllä yhtään jaksaisi syödä ribale räiskissejä
				/aliases 										listaa kaikki käytössä olevat aliakset

			<ei komentoa>									lähetä normaali viesti nykyisen ikkunan kanavalle
			<ALT + [0...9]>									vaihda nykyistä ikkunaa. Yhdessä ikkunassa yksi kanava, ensimmäisessä ikkunassa
															serverin tulostamat kakkapasketit, aivan kuten esim Irssissä

		- käynnistyksen yhteydessä lue tiedostoon tallennetut aiemmat asetukset (.ini-tiedosto. tämän voi tehdä viimeisenä)


HUOMIOITAVAA KEHITYKSESSÄ:

- tekstin muotoilu:
	- server vastaa siitä, että viestit ja /actionit lähetetään kaikille clienteille VALMIIKSI MUOTOILTUNA.
	- client vastaa omien toimintojensa muotoilusta muilta osin
	- client muotoilee aliaksensa myös omaan näkymäänsä

- hyödynnettäviä tietorakenteita:
	- struct
	- linked list
		- jos elementtien lukumäärä tuntematon (vrt. array, jonka tarpeellista kokoa et tiedä etukäteen), voit tehdä linked listin
			- koska koodataan C:tä, linked list on hyvä osata luoda rakenteena itse structin ja pointterien avulla
				- ei valmiita makroja tai kirjastoja netistä, vaan tutustu rakenteeseen esim tästä:
					https://urlzs.com/gpWhh

- suunnittele projekti ennen yhtäkään koodiriviä
	- mitkä ovat ensimmäiseksi rakennettavia, helposti testattavia mukavia pikkupalikoita?
	- mitä functioita, rakenteita, testejä ym kannattaa tehdä ennen tietyn kokonaisuuden rakentamista?
	- mikä olisi helppo kirjasto toteuttaa TCP-yhteyskäytäntö?
		- tätä ennen voi jo tehdä todella paljon palasia valmiiksi(!)
