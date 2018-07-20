import _ from 'lodash';
import '../styles/main.scss';
import Icon from '../assets/garasubo.jpg';

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

