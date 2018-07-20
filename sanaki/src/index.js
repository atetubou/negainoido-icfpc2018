import _ from 'lodash';
import '../styles/main.scss';
import Icon from '../assets/garasubo.jpg';
import React from 'react';
import ReactDOM from 'react-dom';
import Sample from './component/Sample';


const component = () => {
  let element = document.createElement('div');

  element.innerHTML = _.join(['Hello', 'webpack'], ' ');

  let img = new Image();
  img.src = Icon;
  img.className = 'garasubo';

  element.appendChild(img);

  return element;
};

console.log('java');

document.body.appendChild(component());
console.log(document.getElementById('#reactSample'));

ReactDOM.render(<Sample />, document.getElementById('reactSample'));
