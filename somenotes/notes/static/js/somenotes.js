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
		// Based on the solution provided by Mozilla:
		// https://developer.mozilla.org/en-US/docs/Glossary/Base64
		OfdxSomeNotes.atob = function(b64){
			const bin = atob(b64);
			return (new TextDecoder().decode(Uint8Array.from(bin, (m) => m.codePointAt(0))));
		};
		OfdxSomeNotes.btoa = function(str){
			const bytes = new TextEncoder().encode(str);
			const bin = Array.from(bytes, (b) => String.fromCodePoint(b)).join("");
			return btoa(bin);
		};


		var note_texted_field = document.getElementById('note_texted_field');
		var note_texted_title = document.getElementById('note_texted_doc_title');

		// Preload the field with the last value from localStorage, but disable
		// it until we get the latest from the server.
		note_texted_field.value = localStorage.getItem('note_texted_field_value');

		// Get the latest version of the last note from the server
		OfdxSomeNotes.loadNote = function(notename){
			note_texted_field.disabled = true;

			if(notename){
				localStorage.setItem('note_texted_lastfile', notename);

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
							note_texted_field.value = OfdxSomeNotes.atob(http.response.body);
							note_texted_title.value = OfdxSomeNotes.atob(http.response.title);
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
		}
		OfdxSomeNotes.loadNote(localStorage.getItem('note_texted_lastfile'));

		// Store changes.
		var changeTimeout = undefined;
		var changeFunction = function(e){
			if(changeTimeout){
				clearTimeout(changeTimeout);
			}

			changeTimeout = setTimeout(function(){
				changeTimeout = undefined;

				// Store locally just in case.
				localStorage.setItem('note_texted_field_value', note_texted_field.value);

				// Store remotely if this is a named note.
				let notename = localStorage.getItem('note_texted_lastfile');
				if(notename){
					let putbody = {
						body: OfdxSomeNotes.btoa(note_texted_field.value),
						title: OfdxSomeNotes.btoa(note_texted_title.value)
					};

					OfdxAsync.send({
						target: 'f/' + notename,
						method: 'PUT',
						type: 'json',
						post: JSON.stringify(putbody),
						ondone: function(http){
							// FIXME debug
							console.log('PUT modified note:', http);

							// TODO - handle failed to PUT update
						}
					});
				}
			}, 500);
		}

		// TODO - separate this out so we only send the changed field instead of both.
		note_texted_field.addEventListener('input', changeFunction);
		note_texted_title.addEventListener('input', changeFunction);
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
								return { ... el, title: OfdxSomeNotes.atob(el.title) };
							})
							// Sort from most to least recently modified.
							.sort((a, b) => a.modified < b.modified);

						// List all available files.

						// FIXME debug
						card.innerHTML = "";
						for(let i = 0; i < OfdxSomeNotes.notes.length; ++ i){
							let n = OfdxSomeNotes.notes[i];
							let el = document.createElement('div');

							// TODO - style this, present modification time, short preview, handle the title being unset, etc.
							el.classList.add('test');
							el.innerHTML = `<span>${n.id}</span> | <span>${n.title}</span>`;

							card.appendChild(el);

							el.addEventListener('click', function(){
								OfdxSomeNotes.loadNote(n.id);
								OfdxSomeNotes.popAll();
							});
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
