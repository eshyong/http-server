<?php
    $name = $_GET["name"];
    $age = $_GET["age"];
    if ($name && $age) {
        echo "Welcome " . $name . "<br />";
        echo "You are " . $age . " years old.";
        exit();
    }
    echo "Hello!";
?>
