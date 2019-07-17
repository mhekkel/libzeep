const CleanWebpackPlugin = require('clean-webpack-plugin');
const UglifyJsPlugin = require('uglify-js-plugin');
const webpack = require("webpack");

const SCRIPTS = __dirname + "/webapp/";
const DEST = __dirname + "/docroot/scripts/";

module.exports = (env) => {

	const webpackConf = {
		entry: {
			polyfill: '@babel/polyfill',
			opname: SCRIPTS + "opname.js",
			grafiek: SCRIPTS + "grafiek.js"
		},

		module: {
			rules: [
				{
					test: /\.js$/,
					exclude: /node_modules/,
					use: {
						loader: "babel-loader",
						options: {
							presets: ['@babel/preset-env']
						}
					}
				},
				{
					test: /\.css$/,
					use: ['style-loader', 'css-loader']
				},
				{
					test: /\.(eot|svg|ttf|woff(2)?)(\?v=\d+\.\d+\.\d+)?/,
					loader: 'file-loader',
					options: {
						name: '[name].[ext]',
						outputPath: '../fonts/',
						publicPath: 'fonts/'
					}
				}
			]
		},

		output: {
			path: DEST,
			filename: "[name].js"
		},

		plugins: [
			new CleanWebpackPlugin(),
			new webpack.ProvidePlugin({
				$: 'jquery',
				jQuery: 'jquery'
			})
		]

	};

//	const PRODUCTIE = [1, '1', 'true'].includes(env.PRODUCTIE);
	const PRODUCTIE = false;

	if (PRODUCTIE) {
		webpackConf.mode = "production";
		webpackConf.plugins.push(new UglifyJsPlugin({parallel: 4}))
	} else {
		webpackConf.mode = "development";
		webpackConf.devtool = 'source-map';
		webpackConf.plugins.push(new webpack.optimize.AggressiveMergingPlugin())
	}

	return webpackConf;
};
