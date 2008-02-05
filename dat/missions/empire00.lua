

function create()

	-- Intro text
	tk.msg("Spaceport Bar",
[[As you enter the bar you can't help to notice that a fellow at a table is looking at you since you came in.  You tend to your business as if you hadn't noticed.  A while later you feel a tap on your shoulder and see it's the same fellow.]]);
	if tk.yesno("Spaceport Bar",
[["Hello, sorry about interrupting you.  I'm Lieutenant Czesc from the Empire Armada.  We're having another recruitment operation and would be interested in having another pilot among us.  Would be interesting in working for the Empire?"]])
		then
		misn.accept()

		dest = space.getPlanet("Empire");

		-- Mission details
		misn.setTitle("Empire Recruitment")
		misn.setReward("3000 credits")
		misn.setDesc( string.format("Deliver some parcels for the Empire to %s.",dest))

		-- Flavour text and mini-briefing
		tk.msg("Empire Recruitment", string.format(
[["Welcome aboard.", says Czesc before giving you a firm handshake.  "At first you'll just be tested with cargo missions while we get data on your flying skills.  Later you could get called for more important mission.  Who knows?  You could be the next Yao Pternov, greatest pilot we ever had on the armada."
He hits a couple buttons on his wrist computer that springs into action.
"It looks like we already have a simple task for you. Deliver these parcels to %s.  The best pilots started delivering papers and ended up flying into combat against gigantic warships."]],dest))

		-- Set up the goal
		parcels = player.addCargo("Parcels", 0)
		hook.land("land")
	end
end


function land()
	if space.landName() == dest then
		if player.rmCargo(parcels) then
			player.pay(3000)
			-- More flavour text
			tk.msg("Mission Success", string.format(
"You deliver the parcels to the Empire station at the %s spaceport.  Afterwards they make you do some paperwork to formalize your participation with the Empire.  You aren't too sure of what to make of your encounter with the Empire, only time will tell",dest))
			misn.finish()
		end
	end
end

