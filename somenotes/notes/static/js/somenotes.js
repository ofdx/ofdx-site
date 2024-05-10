/*
	somenotes.js
	mperron (2024)
*/

var OfdxSomeNotes = {
	note: {},
	notes: [],
	cardstack: []
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
				ondone: function(http){
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

			pick.menuState = function(state){
				let target = pick.hasAttribute('target-el') ? document.getElementById(pick.getAttribute('target-el')) : undefined;

				if(state === undefined){
					// Default behavior is toggle.
					if(target)
						target.classList.toggle('active');

					pick.classList.toggle('active');
				} else if(state){
					// Show
					if(target)
						target.classList.add('active');

					pick.classList.add('active');
				} else {
					// Hide
					if(target)
						target.classList.remove('active');

					pick.classList.remove('active');
				}
			}

			pick.addEventListener('click', function(){
				// Open this menu if it wasn't already open.
				pick.menuState();

				for(let ii = 0; ii < ctrlbar_picks.length; ++ ii){
					let other = ctrlbar_picks[ii];

					// Remove pick class and close its menu.
					if(other !== pick)
						other.menuState(false);
				}
			});
		}

		// Close the menu bar if we get a click in the main card area.
		var note_card_area = document.getElementById('note_card_area');
		if(note_card_area){
			note_card_area.addEventListener('click', function(){
				for(let i = 0; i < ctrlbar_picks.length; ++ i)
					ctrlbar_picks[i].menuState(false);
			});
		}

		function onCardLoadShow(card){
			card.innerHTML = "<p>Loading...</p>";

			// Get all notes for this user and populate the dialog.
			OfdxAsync.send({
				target: 'f/',
				type: 'json',
				ondone: function(http){
					if(http.status === 200){
						OfdxSomeNotes.notes = http.response.notes
							// Decode base64 title.
							.map(el => {
								return { ... el, title: atob(el.title) };
							})
							// Sort from most to least recently modified.
							.sort((a, b) => a.modified < b.modified);

						// List all available files.

						// FIXME debug
						card.innerHTML = "";
						for(let i = 0; i < OfdxSomeNotes.notes.length; ++ i){
							let n = OfdxSomeNotes.notes[i];
							let el = document.createElement('div');

							el.classList.add('test');
							el.innerHTML = `<span>${n.id}</span> | <span>${n.title}</span>`;

							card.appendChild(el);
						}

						// Link to close this dialog.
						{
							let backbtn = document.createElement('a');

							backbtn.textContent = 'Back to Editor';
							backbtn.classList.add('btn');
							backbtn.addEventListener('click', function(){
								OfdxSomeNotes.popAll();
							});

							card.appendChild(backbtn);
						}

					} else {
						// FIXME debug
						alert('notes not loaded...');
					}
				},
				onerror: function(http){
					// FIXME debug
					console.log('error f/!', http);
				}
			});
		}

		// Push a card onto the stack, make it active, and call the show event handler.
		OfdxSomeNotes.pushCard = function(card){
			let cards = document.querySelectorAll('.note_texted_card.active');

			for(let i = 0; i < cards.length; ++ i){
				let other = cards[i];

				if(other !== card){
					// Push the currently active card to the stack so we can get back to it.
					OfdxSomeNotes.cardstack.push(other);
					other.classList.remove('active');
					break;
				}
			}

			if(!card.classList.contains('active')){
				card.classList.add('active');

				switch(card.getAttribute('id')){
					case 'card_load':
						onCardLoadShow(card);
						break;
				}
			}
		}

		// Return to the previous card.
		OfdxSomeNotes.popCard = function(){
			let cards = document.querySelectorAll('.note_texted_card.active');
			let card = OfdxSomeNotes.cardstack.pop();

			// Only pop if there is something in the stack. Otherwise the whole
			// application will disappear. :)
			if(card){
				for(let i = 0; i < cards.length; ++ i)
					cards[i].classList.remove('active');

				card.classList.add('active');
			}
		}

		// Return to the top-level card.
		OfdxSomeNotes.popAll = function(){
			if(OfdxSomeNotes.cardstack.length > 0){
				let cards = document.querySelectorAll('.note_texted_card.active');
				let card = OfdxSomeNotes.cardstack[0];

				for(let i = 0; i < cards.length; ++ i)
					cards[i].classList.remove('active');

				card.classList.add('active');
				OfdxSomeNotes.cardstack = [];
			}
		}

		var menu_picks = document.querySelectorAll(".note_texted_ctrlbar_menu .pick");
		for(let i = 0; i < menu_picks.length; ++ i){
			let pick = menu_picks[i];
			let target_card = pick.hasAttribute('target-card') ? document.getElementById(pick.getAttribute('target-card')) : undefined;

			pick.addEventListener('click', function(){
				// Close the menu since we made a selection.
				let menus = document.querySelectorAll(".note_texted_ctrlbar_menu.active, #note_texted_ctrlbar .pick.active");
				for(let ii = 0; ii < menus.length; ++ ii)
					menus[ii].classList.remove('active');

				if(target_card)
					OfdxSomeNotes.pushCard(target_card);
			})
		}

	}
});
