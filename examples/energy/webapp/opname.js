import '@babel/polyfill'
import '@fortawesome/fontawesome-free/css/all.min.css';
import 'bootstrap';
import 'bootstrap/js/dist/modal'
import 'bootstrap/dist/css/bootstrap.min.css';

class OpnameEditor {

	constructor() {
		this.dialog = document.getElementById("opname-dialog");
		this.form = document.getElementById("opname-edit-form");
		// this.csrf = this.form.elements['_csrf'].value;

		this.form.addEventListener("submit", (evt) => this.saveOpname(evt));
	}

	editOpname(id) {
		this.id = id;

		$(this.dialog).modal();

		fetch(`ajax/opname/${id}`, {credentials: "include", method: "get"})
			.then(async response => {
				if (response.ok)
					return response.json();

				const error = await response.json();
				console.log(error);
				throw error.message;
			})
			.then(data => {

				this.opname = data;

				for (let key in data.standen) {
					const input = document.getElementById(`id-${key}`);
					input.value = data.standen[key];
				}
			})
			.catch(err => alert(err));
	}

	saveOpname(e) {
		if (e)
			e.preventDefault();

		this.opname.standen = {};
		for (let key in this.form.elements) {
			const input = this.form.elements[key];
			if (input.type !== 'number') continue;
			this.opname.standen[`${input.dataset.id}`] = +input.value;
		}

		const url = this.id ? `ajax/opname/${this.id}` : 'ajax/opname';
		const method = this.id ? 'put' : 'post';

		fetch(url, {
			credentials: "include",
			headers: {
				'Accept': 'application/json',
				'Content-Type': 'application/json',
				// 'X-CSRF-Token': this.csrf
			},
			method: method,
			body: JSON.stringify(this.opname)
		}).then(async response => {
			if (response.ok)
				return response.json();

			const error = await response.json();
			console.log(error);
			throw error.message;
		}).then(r => {
			console.log(r);
			$(this.dialog).modal('hide');

			window.location.reload();
		}).catch(err => alert(err));
	}

	createOpname() {
		this.id = null;
		this.opname = {};
		this.form.reset();
		$(this.dialog).modal();
	}

	deleteOpname(id, name) {
		if (confirm(`Are you sure you want to delete opname ${name}?`)) {
			fetch(`ajax/opname/${id}`, {
				credentials: "include",
				method: "delete",
				headers: {
					'Accept': 'application/json',
					// 'Content-Type': 'application/json',
					// 'X-CSRF-Token': this.csrf
				}
			}).then(async response => {
				if (response.ok)
					return response.json();

				const error = await response.json();
				console.log(error);
				throw error.message;
			}).then(data => {
				console.log(data);

				window.location.reload();
			})
				.catch(err => alert(err));
		}
	}
}

window.addEventListener("load", () => {

	const editor = new OpnameEditor();

	Array.from(document.getElementsByClassName("edit-opname-btn"))
		.forEach(btn => btn.addEventListener("click", () => editor.editOpname(btn.dataset.id)));

	Array.from(document.getElementsByClassName("delete-opname-btn"))
		.forEach(btn => btn.addEventListener("click", () => {
			return editor.deleteOpname(btn.dataset.id, btn.dataset.name);
		}));

	document.getElementById("add-opname-btn")
		.addEventListener("click", () => editor.createOpname());


	// Array.from(document.getElementById('opname-table').tBodies[0].rows)
	// 	.forEach(tr => {
	// 		tr.addEventListener("dblclick", () => {
	// 			editor.editOpname(tr.dataset.uid);
	// 		})
	// 	});
});
