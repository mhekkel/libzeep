import '@babel/polyfill'
import '@fortawesome/fontawesome-free/css/all.min.css';
import 'bootstrap';
import 'bootstrap/js/dist/modal'
import 'bootstrap/dist/css/bootstrap.min.css';
import * as d3 from 'd3';

class grafiek {
	constructor() {

		this.selector = document.getElementById('grafiek-id');
		this.selector.addEventListener('change', ev => {
			if (ev)
				ev.preventDefault();
			this.laadGrafiek();
		});

		for (let btn of document.getElementsByClassName("aggr-btn")) {
			if (btn.checked)
				this.aggrType = btn.dataset.aggr;
			btn.onchange = (e) => this.selectAggrType(e.target.dataset.aggr);
		}

		this.svg = d3.select("svg");

		const plotContainer = $(this.svg.node());
		let bBoxWidth = plotContainer.width();
		let bBoxHeight = plotContainer.height();

		if ((bBoxWidth * 9 / 16) > bBoxHeight)
			bBoxWidth = 16 * bBoxHeight / 9;
		else
			bBoxHeight = 9 * bBoxWidth / 16;

		this.margin = {top: 30, right: 50, bottom: 30, left: 50};

		this.width = bBoxWidth - this.margin.left - this.margin.right;
		this.height = bBoxHeight - this.margin.top - this.margin.bottom;

		this.defs = this.svg.append('defs');

		this.defs
			.append("svg:clipPath")
			.attr("id", "clip")
			.append("svg:rect")
			.attr("x", 0)
			.attr("y", 0)
			.attr("width", this.width)
			.attr("height", this.height);

		this.svg.append("text")
			.attr("class", "x axis-label")
			.attr("text-anchor", "end")
			.attr("x", this.width + this.margin.left)
			.attr("y", this.height + this.margin.top + this.margin.bottom)
			.text("Datum");

		this.svg.append("text")
			.attr("class", "y axis-label")
			.attr("text-anchor", "end")
			.attr("x", -this.margin.top)
			.attr("y", 6)
			.attr("dy", ".75em")
			.attr("transform", "rotate(-90)")
			.text("Verbruik per dag");

		this.g = this.svg.append("g")
			.attr("transform", `translate(${this.margin.left},${this.margin.top})`);

		this.gX = this.g.append("g")
			.attr("class", "axis axis--x")
			.attr("transform", `translate(0,${this.height})`);

		this.gY = this.g.append("g")
			.attr("class", "axis axis--y");

		this.plot = this.g.append("g")
			.attr("class", "plot")
			.attr("width", this.width)
			.attr("height", this.height)
			.attr("clip-path", "url(#clip)");

		this.plotData = this.plot.append('g')
			.attr("width", this.width)
			.attr("height", this.height);

		const kleurLijn = '#1f78b4';
		const kleurArea = '#a6cee3';

		console.log([kleurLijn, kleurArea]);

		this.lijn = this.plotData.append('path')
			.attr('class', 'grafiek-lijn')
			.attr('stroke', kleurLijn)
			.attr('stroke-width', 2)
			.attr('fill', 'none');

		this.area = this.plotData.append('path')
			.attr('class', 'area')
			.attr('fill', kleurArea);

		const zoom = d3.zoom()
			.scaleExtent([1, 40])
			.translateExtent([[0, 0], [this.width + 90, this.height + 90]])
			.on("zoom", () => this.zoomed());

		this.svg.call(zoom);
	}
	
	laadGrafiek() {
		const selected = this.selector.selectedOptions;
		if (selected.length !== 1)
			reject("ongeldige keuze");

		const keuze = selected.item(0).value;
		const grafiekNaam = selected.item(0).textContent;

		let grafiekTitel = $(".grafiek-titel");
		if (grafiekTitel.hasClass("grafiek-status-loading"))  // avoid multiple runs
			return;

		grafiekTitel
			.addClass("grafiek-status-loading")
			.removeClass("grafiek-status-loaded")
			.removeClass("grafiek-status-failed");

		Array.from(document.getElementsByClassName("grafiek-naam"))
			.forEach(span => span.textContent = grafiekNaam);

		fetch(`ajax/data/${keuze}/${this.aggrType}`, {
			credentials: "include",
			headers: {
				'Accept': 'application/json'
			}
		}).then(async response => {
			if (response.ok)
				return response.json();

			const error = await response.json();
			console.log(error);
			throw error.message;
		}).then(data => {
			this.processData(data);
			grafiekTitel.removeClass("grafiek-status-loading").addClass("grafiek-status-loaded");
		}).catch(err => {
			grafiekTitel.removeClass("grafiek-status-loading").addClass("grafiek-status-failed");
			console.log(err);
		});
	}

	processData(data) {
		
		const dataPunten = [];
		for (let d in data.punten)
		{
			dataPunten.push({
				date: new Date(d),
				verbruik: data.punten[d]
			});
		}

		// const dataPunten = data.standen.entries()
		// 	.map(e => {
		// 		[datum, verbruik] = e;
		// 		return {
		// 			date: new Date(datum),
		// 			verbruik: verbruik
		// 		};
		// 	});

		// const dataPunten = data.punten.sort((a, b) => {
		// 	let d = a.datum[0] - b.datum[0];
		// 	if (d === 0) d = a.datum[1] - b.datum[1];
		// 	if (d === 0) d = a.datum[2] - b.datum[2];
		// 	return d;
		// });

		// dataPunten.forEach(d => d.date = new Date(...d.datum));

		const domX = d3.extent(dataPunten, d => d.date);

		const x = d3.scaleTime().domain(domX).range([1, this.width - 2]);

		const xAxis = d3.axisBottom(x);

		this.gX.call(xAxis);

		const domY = d3.extent(dataPunten, d => d.verbruik);

		const y = d3.scaleLinear().domain(domY).range([this.height - 2, 1]);

		const yAxis = d3.axisLeft(y);

		this.gY.call(yAxis);

		const line = d3.line()
			.x(d => x(d.date))
			.y(d => y(d.verbruik));

		this.lijn
			.datum(dataPunten)
			.attr('d', line);

		if (domY[0] < 0 && domY[1] > 0) {

			const area = d3.area()
				.x(d => x(d.date))
				.y0(d => y(0))
				.y1(d => y(d.verbruik));

			this.area
				.datum(dataPunten)
				.attr('d', area);
		}
		else
			this.area.attr('d', null);
	}

	zoomed() {

	}

	selectAggrType(aggr) {
		this.aggrType = aggr;
		this.laadGrafiek()
	}
}


window.addEventListener("load", () => {
	const g = new grafiek();
	g.laadGrafiek();
});
