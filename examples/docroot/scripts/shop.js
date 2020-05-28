/*
	simple shopping cart example
*/

class ShoppingCart {
	constructor(name, id) {
		this.name = name;
		this.cartID = id;

		const accountForm = document.getElementById('account-form');
		accountForm.style.display = 'none';

		const shoppingPage = document.getElementById('shopping-page');
		shoppingPage.style.display = 'unset';

		document.getElementById('user-name').textContent = name;

		[...document.getElementsByClassName('shopping-item')]
			.forEach(item => {
				item.addEventListener('click', (evt) => {
					evt.preventDefault();
					this.addToCart(item.dataset.item);
				})
			});
	}

	addToCart(item) {
		const fd = new FormData();
		fd.append("name", item);
		fetch(`/cart/${this.cartID}/item`, { method: "POST", body: fd})
			.then(r => r.json())
			.then(order => this.updateOrder(order))
			.catch(err => {
				console.log(err);
				alert(`Failed to add ${item} to cart`);
			});
	}

	deleteCartItem(item) {
		const fd = new FormData();
		fd.append("name", item);
		fetch(`/cart/${this.cartID}/item`, { method: "DELETE", body: fd})
			.then(r => r.json())
			.then(order => this.updateOrder(order))
			.catch(err => {
				console.log(err);
				alert(`Failed to remove ${item} from cart`);
			});
	}

	updateOrder(order) {
		const cartListContainer = document.getElementById('cart-list-container');
		
		if (order.items.length == 0)
			cartListContainer.style.display = 'none';
		else
		{
			cartListContainer.style.display = 'initial';

			const cartList = document.getElementById('cart-list');
			[...cartList.querySelectorAll('li:not(:first-child)')]
				.forEach(li => li.remove());
			
			const li = cartListContainer.querySelector('li');

			order.items.forEach(item => {
				const lic = li.cloneNode(true);
				lic.querySelector('span.text-placeholder').textContent = `${item.name} (${item.count})`;
				const removeBtn = lic.querySelector('span.cart-item');
				removeBtn.addEventListener('click', (li) => this.deleteCartItem(item.name));
				cartList.append(lic);
			});
		}

		console.log(order);
	}
};

window.addEventListener('load', () => {
	const createCartBtn = document.getElementById('create-cart-btn');
	createCartBtn.addEventListener('click', (evt) => {
		evt.preventDefault();
		
		const client = document.forms['client-form']['client-name'].value;

		if (client == "")
			alert("Please enter a user name");
		else
		{
			fetch('/cart', { method: "POST" })
				.then(r => r.json())
				.then(cartID => new ShoppingCart(client, cartID))
				.catch(err => {
					console.log(err);
					alert("failed to create shopping cart");
				})
		}
	});
});