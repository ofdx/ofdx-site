/*
	somenotes.js
	mperron (2024)
*/

var OfdxSomeNotes = {
	note: {}
};

window.addEventListener('load', function(){
	// Main textarea
	{
		var note_texted_field = document.getElementById('note_texted_field');
		var note_texted_title = document.getElementById('note_texted_doc_title');

		// Preload the field with the last value from localStorage, but disable
		// it until we get the latest from the server.
		note_texted_field.value = localStorage.getItem('note_texted_field_value');
		note_texted_field.disabled = true;

		// Get the latest version of the last note from the server
		let notename = localStorage.getItem('note_texted_lastfile');
		if(notename){
			OfdxAsync.send({
				target: 'f/' + notename,
				type: 'json',
				completion: function(http){
					// FIXME debug
					console.log(http);

					if(http.status === 200){
						// Store this response for future reference.
						OfdxSomeNotes.note = http.response;

						// Update fields on the page.
						note_texted_field.value = atob(http.response.body);
						note_texted_title.value = atob(http.response.title);
					} else {
						// FIXME debug
						alert('note not loaded...');

						// TODO handle if file does not exist on the server
					}

					note_texted_field.disabled = false;
				},
				onerror: function(http){
					// FIXME debug
					console.log('error!', http);
				}
			});
		} else {
			note_texted_field.disabled = false;
		}

		// Store changes.
		note_texted_field.addEventListener('change', function(e){
			// Store locally immediately.
			localStorage.setItem('note_texted_field_value', note_texted_field.value);

			// Store remotely after a timeout to minimize server impact.
			// TODO
		});
	}

	// Menubar
	{
		var ctrlbar_picks = document.querySelectorAll("#note_texted_ctrlbar .pick"); 

		for(let i = 0; i < ctrlbar_picks.length; ++ i){
			let pick = ctrlbar_picks[i];

			pick.addEventListener('click', function(){
				// Open this menu if it wasn't already open.
				pick.classList.toggle('active');

				for(let ii = 0; ii < ctrlbar_picks.length; ++ ii){
					let other = ctrlbar_picks[ii];

					// Remove pick class and close its menu.
					if(other !== pick)
						other.classList.remove('active');
				}
			});
		}
	}
});
