#!/bin/bash

echo 'Content-Type: text/html'
echo
echo '<h2>Xcache:</h2>'
xcache-stat -w
xcache-list -w

