import unittest
import requests
import json

debug_server='http://localhost:8080'

class TestSelect(unittest.TestCase):
    def test_get(self):
        res = requests.get(debug_server + '/test_select')
        self.assertEqual(res.status_code, 200)
    
    def test_get_json(self):
        res = requests.get(debug_server + '/test_select/json')
        data = json.loads(res.text)
        self.assertEqual(data['result'], 'success')
        self.assertIsInstance(data['value'], list)
        for element in data['value']:
            self.assertIsInstance(element['id'], int)
            res = requests.get(debug_server + '/test_select/' + str(element['id']))
            self.assertEqual(res.status_code, 200)
            self.assertIsInstance(element['created_at'], str)
            self.assertIsInstance(element['message'], str)

    
