/*
	somenotes.js
	mperron (2024)
*/

window.addEventListener('load', function(){
	// Main textarea
	{
		var note_texted_field = document.getElementById('note_texted_field');

		// Save/restore state of text editor.
		note_texted_field.value = localStorage.getItem('note_texted_field_value');
		note_texted_field.addEventListener('change', function(e){
			localStorage.setItem('note_texted_field_value', note_texted_field.value);
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
