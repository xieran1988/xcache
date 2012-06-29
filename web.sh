#!/bin/bash

echo 'Content-Type: text/html'
echo
echo '<h2>Xcache:</h2>'
echo '<div>'
cat /var/lib/xcache-log/cap-stat
echo '</div>'
xcache-stat -w
xcache-list -w

